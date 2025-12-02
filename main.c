// À faire : implémenter un bot simple, détection de pat, nulle par accord mutuel, répétition de position, règle des 50 coups, détection des positions de nulle par manque de matériel (ex. Roi vs Roi, Roi+Fou vs Roi, etc.).
// Remarques : implémenter d'abord les règles de base d'échecs (incomplètes)
// string.h et stdio.h pour printf, fgets, strcmp, strlen pour remplacer scanf et gérer les entrées plus simple avec if
#include <stdio.h>
#include <string.h>
// Variables et types globaux
// struct et enums de type : TypePiece, Couleur, Piece, Coup
typedef enum {VIDE=0,PION,CAVALIER,FOU,TOUR,DAME,ROI} TypePiece;
// Couleur des pièces Aucune=0, Blanc=1, Noir=2, aucune pour cases vides
typedef enum {AUCUNE=0,BLANC=1,NOIR=2} Couleur;
// une pièce : type, couleur, aBouge (utile pour roque et avance de 2 cases du pion)
typedef struct {TypePiece type; Couleur col; int aBouge;} Piece;
// un coup : from_row, from_col, to_row, to_col, promotion_piece (type Piece avec type=VIDE si pas de promo)
typedef struct {int fr,fc,tr,tc; Piece promo;} Coup;
// 8X8 un tableau a-h et 1-8
#define N 8
// variables globales
Piece plateau[N][N];
// position des rois pour vérification rapide
int roi_r_blanc, roi_c_blanc, roi_r_noir, roi_c_noir;
// trait (joueur dont c'est le tour) avec Couleur enum qui vaut BLANC ou NOIR ou AUCUNE
Couleur trait=BLANC;
// case de prise en passant (-1,-1 si pas de prise en passant possible)
int ep_r=-1, ep_c=-1;

// déclaration globale demandée pour remplacer "for(int i=0...)"
int i;

// derniers coups suggérés (pour sélection par numéro) 1024 car c'est 8*8*2*2 pour 1 bot plus tard
Coup last_moves[1024];
int last_moves_n = 0;

// fonctions
void init_plateau(); void afficher(); const char* icone(Piece p);
int sur_plateau(int r,int c); int coup_pseudo_legal(Coup m,Couleur mover);
int est_en_echec(Couleur kcol); int laisse_roi_en_echec(Coup m,Couleur mover);
int coup_legal(Coup m,Couleur mover); int gen_tous(Couleur c,Coup out[],int max);
void appliquer(Coup m); int parse_coup(const char *s,Coup *m);
void suggerer(); int est_echec_et_mat(Couleur c); void copier(Piece dst[N][N],Piece src[N][N]);

// utilitaires
int sur_plateau(int r,int c){return r>=0&&r<8&&c>=0&&c<8;}
int fichier_col(char f){return f-'a';}
int rang_lig(char r){return 8-(r-'0');}

// initialisation
void set_piece(int r,int c,TypePiece t,Couleur co){
    plateau[r][c].type=t; plateau[r][c].col=co; plateau[r][c].aBouge=0;
    if(t==ROI){if(co==BLANC){roi_r_blanc=r; roi_c_blanc=c;} else {roi_r_noir=r; roi_c_noir=c;}}
}

void init_plateau(){
    int r,c; for(r=0;r<N;r++) for(c=0;c<N;c++){ plateau[r][c].type=VIDE; plateau[r][c].col=AUCUNE; plateau[r][c].aBouge=0;}
    // noirs
    set_piece(0,0,TOUR,NOIR); set_piece(0,1,CAVALIER,NOIR); set_piece(0,2,FOU,NOIR); set_piece(0,3,DAME,NOIR);
    set_piece(0,4,ROI,NOIR); set_piece(0,5,FOU,NOIR); set_piece(0,6,CAVALIER,NOIR); set_piece(0,7,TOUR,NOIR);
    for(c=0;c<N;c++) set_piece(1,c,PION,NOIR);
    // blancs
    set_piece(7,0,TOUR,BLANC); set_piece(7,1,CAVALIER,BLANC); set_piece(7,2,FOU,BLANC); set_piece(7,3,DAME,BLANC);
    set_piece(7,4,ROI,BLANC); set_piece(7,5,FOU,BLANC); set_piece(7,6,CAVALIER,BLANC); set_piece(7,7,TOUR,BLANC);
    for(c=0;c<N;c++) set_piece(6,c,PION,BLANC);
    trait=BLANC; ep_r=ep_c=-1;
}

// icones d'echec pris par: https://fr.wikipedia.org/wiki/Symboles_d%27%C3%A9checs_en_Unicode
const char* icone(Piece p){
    if(p.type==VIDE) return ".";
    if(p.col==NOIR){switch(p.type){case PION:return"♙";case CAVALIER:return"♘";case FOU:return"♗";case TOUR:return"♖";case DAME:return"♕";case ROI:return"♔";default:return"?";}}
    else{switch(p.type){case PION:return"♟";case CAVALIER:return"♞";case FOU:return"♝";case TOUR:return"♜";case DAME:return"♛";case ROI:return"♚";default:return"?";}}
}

// affichage une ligne par rangée, motif clair/sombre avec 2 espaces et ::
void afficher(){
    int r,c;
    printf("\n\n\n     a b c d e f g h\n\n");
    for(r=0;r<N;r++){
        printf(" %d   ", 8-r);
        for(c=0;c<N;c++){
            Piece p = plateau[r][c];
            if(p.type==VIDE){
                // motif clair/sombre pour cases vides
                if(((r+c)&1)==0) printf("  "); else printf("::");
            } else {
                // afficher la pièce
                printf("%s ", icone(p));
            }
        }
        printf("   %d\n", 8-r);
    }
    printf("\n     a b c d e f g h\n");
}

// copie plateau plus simple comme ca
void copier(Piece dst[N][N],Piece src[N][N]){int r,c;for(r=0;r<N;r++)for(c=0;c<N;c++) dst[r][c]=src[r][c];}

// vérifie si roi est attaqué
int est_en_echec(Couleur kcol){
    int kr=(kcol==BLANC)?roi_r_blanc:roi_r_noir;
    int kc=(kcol==BLANC)?roi_c_blanc:roi_c_noir;
    Couleur adv=(kcol==BLANC)?NOIR:BLANC;
    int r,c; for(r=0;r<N;r++) for(c=0;c<N;c++){
        Piece p=plateau[r][c]; if(p.type==VIDE||p.col!=adv) continue;
        Coup m={r,c,kr,kc,{VIDE,AUCUNE,0}}; if(coup_pseudo_legal(m,adv)) return 1;
    }
    return 0;
}

// vérifie si roi reste en échec après coup
int laisse_roi_en_echec(Coup m,Couleur mover){
    Piece tmp[N][N]; copier(tmp,plateau);
    Piece src=plateau[m.fr][m.fc]; plateau[m.tr][m.tc]=plateau[m.fr][m.fc]; plateau[m.tr][m.tc].aBouge=1;
    plateau[m.fr][m.fc].type=VIDE; plateau[m.fr][m.fc].col=AUCUNE;
    // gérer prise en passant dans la simulation: si un pion se déplace en diagonale vers une case vide,
    // cela peut être une prise en passant ; la pièce capturée est à (m.fr, m.tc)
    if(src.type==PION && m.fc!=m.tc && tmp[m.tr][m.tc].type==VIDE){
        plateau[m.fr][m.tc].type=VIDE; plateau[m.fr][m.tc].col=AUCUNE;
    }
    // gérer roque dans la simulation : déplacer la tour virtuelle si le roi se déplace de 2 colonnes
    if(src.type==ROI && (m.tc - m.fc==2 || m.tc - m.fc==-2)){
        int rook_src = (m.tc>m.fc)?7:0;
        int rook_dst = (m.tc>m.fc)?m.tc-1:m.tc+1;
        if(sur_plateau(m.fr,rook_src) && tmp[m.fr][rook_src].type==TOUR && tmp[m.fr][rook_src].col==src.col){
            plateau[m.fr][rook_dst]=tmp[m.fr][rook_src]; plateau[m.fr][rook_dst].aBouge=1;
            plateau[m.fr][rook_src].type=VIDE; plateau[m.fr][rook_src].col=AUCUNE;
        }
    }
    if(plateau[m.tr][m.tc].type==ROI){if(plateau[m.tr][m.tc].col==BLANC){roi_r_blanc=m.tr; roi_c_blanc=m.tc;}else{roi_r_noir=m.tr; roi_c_noir=m.tc;}}
    int echec=est_en_echec(mover); copier(plateau,tmp);
    roi_r_blanc=src.type==ROI&&src.col==BLANC?m.fr:roi_r_blanc;
    roi_c_blanc=src.type==ROI&&src.col==BLANC?m.fc:roi_c_blanc;
    roi_r_noir=src.type==ROI&&src.col==NOIR?m.fr:roi_r_noir;
    roi_c_noir=src.type==ROI&&src.col==NOIR?m.fc:roi_c_noir;
    return echec;
}

// pseudo légal pour chaque pièce
int coup_pseudo_legal(Coup m,Couleur mover){
    if(!sur_plateau(m.fr,m.fc)||!sur_plateau(m.tr,m.tc)) return 0;
    Piece src=plateau[m.fr][m.fc], dst=plateau[m.tr][m.tc];
    if(src.type==VIDE||src.col!=mover) return 0;
    if(dst.type!=VIDE&&dst.col==mover) return 0;
    int dr=m.tr-m.fr, dc=m.tc-m.fc, adr=dr<0?-dr:dr, adc=dc<0?-dc:dc;
    switch(src.type){
        case PION:{
            int dir=(src.col==BLANC)?-1:1;
            // avance d'une case
            if(dc==0 && dr==dir && dst.type==VIDE) return 1;
            // avance de deux cases depuis la position initiale
            if(dc==0 && dr==2*dir && dst.type==VIDE){
                int startRank = (src.col==BLANC)?6:1;
                int midr = m.fr + dir;
                if(m.fr==startRank && plateau[midr][m.fc].type==VIDE) return 1;
            }
            // capture normale
            if(dr==dir && adc==1 && dst.type!=VIDE && dst.col!=src.col) return 1;
            // prise en passant: déplacer en diagonale vers une case vide qui est la case ep
            if(dr==dir && adc==1 && dst.type==VIDE && m.tr==ep_r && m.tc==ep_c) return 1;
            return 0;
        }
        case CAVALIER: return (adr==2&&adc==1)||(adr==1&&adc==2);
        case FOU: if(dr==0||dc==0||adr!=adc) return 0; {int r=m.fr+(dr>0?1:-1),c=m.fc+(dc>0?1:-1); while(r!=m.tr){if(plateau[r][c].type!=VIDE) return 0; r+=dr>0?1:-1; c+=dc>0?1:-1;} return 1;}
        case TOUR: if(dr!=0&&dc!=0) return 0; {int r=m.fr+(dr>0?1:dr<0?-1:0),c=m.fc+(dc>0?1:dc<0?-1:0); while(r!=m.tr||c!=m.tc){if(plateau[r][c].type!=VIDE) return 0; r+=dr>0?1:dr<0?-1:0; c+=dc>0?1:dc<0?-1:0;} return 1;}
        case DAME: if(adr==adc){int r=m.fr+(dr>0?1:-1),c=m.fc+(dc>0?1:-1); while(r!=m.tr){if(plateau[r][c].type!=VIDE) return 0; r+=dr>0?1:-1;c+=dc>0?1:-1;} return 1;} if(dr==0||dc==0){int r=m.fr+(dr>0?1:dr<0?-1:0),c=m.fc+(dc>0?1:dc<0?-1:0); while(r!=m.tr||c!=m.tc){if(plateau[r][c].type!=VIDE) return 0; r+=dr>0?1:dr<0?-1:0; c+=dc>0?1:dc<0?-1:0;} return 1;} return 0;
        case ROI: {
            // mouvement normal du roi
            if(adr<=1&&adc<=1) return 1;
            // roque: déplacement de deux colonnes vers la tour
            if(adr==0 && adc==2){
                // le roi ne doit pas avoir bougé
                if(src.aBouge) return 0;
                // déterminer côté
                int rook_col = (m.tc>m.fc)?7:0;
                // vérifier la présence d'une tour de la même couleur et qu'elle n'a pas bougé
                if(!sur_plateau(m.fr,rook_col)) return 0;
                Piece rook = plateau[m.fr][rook_col]; if(rook.type!=TOUR||rook.col!=src.col||rook.aBouge) return 0;
                // vérifier cases intermédiaires vides
                int step = (m.tc>m.fc)?1:-1; int col = m.fc+step;
                while(col!=rook_col){ if(plateau[m.fr][col].type!=VIDE && col!=m.tc) return 0; col+=step; }
                // autoriser ; la validation finale (ne pas passer par l'échec) est faite par coup_legal/laisse_roi_en_echec
                return 1;
            }
            return 0;
        }
        default: return 0;
    }
}

// coup légal complet
int coup_legal(Coup m,Couleur mover){return coup_pseudo_legal(m,mover)&&!laisse_roi_en_echec(m,mover);}

// appliquer coup
void appliquer(Coup m){
    Piece src=plateau[m.fr][m.fc];
    int isEnPassant = 0;
    int doublePush = 0;
    if(src.type==PION){
        if(m.fc!=m.tc && plateau[m.tr][m.tc].type==VIDE && m.tr==ep_r && m.tc==ep_c) isEnPassant=1;
        if((m.tr - m.fr==2) || (m.tr - m.fr==-2)) doublePush=1;
    }
    // déplacer la pièce
    plateau[m.tr][m.tc]=plateau[m.fr][m.fc]; plateau[m.tr][m.tc].aBouge=1;
    plateau[m.fr][m.fc].type=VIDE; plateau[m.fr][m.fc].col=AUCUNE;
    // si roque, déplacer aussi la tour correspondante
    if(src.type==ROI && (m.tc - m.fc==2 || m.tc - m.fc==-2)){
        int rook_src = (m.tc>m.fc)?7:0;
        int rook_dst = (m.tc>m.fc)?m.tc-1:m.tc+1;
        if(sur_plateau(m.fr,rook_src) && plateau[m.fr][rook_src].type==TOUR && plateau[m.fr][rook_src].col==src.col){
            plateau[m.fr][rook_dst]=plateau[m.fr][rook_src]; plateau[m.fr][rook_dst].aBouge=1;
            plateau[m.fr][rook_src].type=VIDE; plateau[m.fr][rook_src].col=AUCUNE;
        }
    }
    // si prise en passant, enlever le pion capturé (il est sur la même rangée que la source et colonne tc)
    if(isEnPassant){ plateau[m.fr][m.tc].type=VIDE; plateau[m.fr][m.tc].col=AUCUNE; }
    // promotion: si le pion atteint la dernière rangée
    if(src.type==PION && (m.tr==0 || m.tr==7)){
        if(m.promo.type!=VIDE){ plateau[m.tr][m.tc].type = m.promo.type; plateau[m.tr][m.tc].col = m.promo.col; }
        else { plateau[m.tr][m.tc].type = DAME; plateau[m.tr][m.tc].col = src.col; }
    } else {
        if(m.promo.type!=VIDE){ plateau[m.tr][m.tc].type = m.promo.type; plateau[m.tr][m.tc].col = m.promo.col; }
    }
    // gérer la case ep (prise en passant possible) : seulement immédiatement après un double-push
    if(doublePush){ ep_r = m.fr + (src.col==BLANC?-1:1); ep_c = m.fc; } else { ep_r = ep_c = -1; }
    if(plateau[m.tr][m.tc].type==ROI){if(plateau[m.tr][m.tc].col==BLANC){roi_r_blanc=m.tr; roi_c_blanc=m.tc;}else{roi_r_noir=m.tr; roi_c_noir=m.tc;}}
}

// générer tous coups légaux
int gen_tous(Couleur side,Coup moves[],int max){
    int count=0,r,c;
    for(r=0;r<N;r++) for(c=0;c<N;c++){
        Piece p=plateau[r][c]; if(p.type==VIDE||p.col!=side) continue;
        if(p.type==PION){
            int dir=(p.col==BLANC?-1:1);
            // avance d'une case
            Coup m={r,c,r+dir,c,{VIDE,AUCUNE,0}};
            if(sur_plateau(m.tr,m.tc) && plateau[m.tr][m.tc].type==VIDE){
                // promotion si atteint la dernière rangée
                if(m.tr==0 || m.tr==7){
                    Coup mm=m; mm.promo.type=DAME; mm.promo.col=side; if(coup_legal(mm,side) && count<max) moves[count++]=mm;
                    mm.promo.type=TOUR; if(coup_legal(mm,side) && count<max) moves[count++]=mm;
                    mm.promo.type=FOU; if(coup_legal(mm,side) && count<max) moves[count++]=mm;
                    mm.promo.type=CAVALIER; if(coup_legal(mm,side) && count<max) moves[count++]=mm;
                } else { if(coup_legal(m,side) && count<max) moves[count++]=m; }
                // avance de deux cases depuis la position initiale
                Coup m2={r,c,r+2*dir,c,{VIDE,AUCUNE,0}}; int startRank = (side==BLANC)?6:1; if(r==startRank){int mid=r+dir; if(sur_plateau(m2.tr,m2.tc) && plateau[m2.tr][m2.tc].type==VIDE && plateau[mid][c].type==VIDE){ if(coup_legal(m2,side) && count<max) moves[count++]=m2; }}
            }
            // captures (incluant promotions)
            for(int dco=-1;dco<=1;dco+=2){
                m.tr = r+dir; m.tc = c+dco;
                if(!sur_plateau(m.tr,m.tc)) continue;
                // capture normale
                if(plateau[m.tr][m.tc].type!=VIDE && plateau[m.tr][m.tc].col!=side){
                    if(m.tr==0 || m.tr==7){ Coup mm=m; mm.promo.type=DAME; mm.promo.col=side; if(coup_legal(mm,side) && count<max) moves[count++]=mm; mm.promo.type=TOUR; if(coup_legal(mm,side) && count<max) moves[count++]=mm; mm.promo.type=FOU; if(coup_legal(mm,side) && count<max) moves[count++]=mm; mm.promo.type=CAVALIER; if(coup_legal(mm,side) && count<max) moves[count++]=mm; }
                    else { m.promo.type=VIDE; m.promo.col=AUCUNE; if(coup_legal(m,side) && count<max) moves[count++]=m; }
                }
                // prise en passant: la case cible est vide mais correspond à ep
                else if(plateau[m.tr][m.tc].type==VIDE && m.tr==ep_r && m.tc==ep_c){ m.promo.type=VIDE; m.promo.col=AUCUNE; if(coup_legal(m,side) && count<max) moves[count++]=m; }
            }
        } else if(p.type==CAVALIER){int kr[8]={2,2,-2,-2,1,1,-1,-1},kc[8]={1,-1,1,-1,2,-2,2,-2}; for(i=0;i<8;i++){Coup m={r,c,r+kr[i],c+kc[i],{VIDE,AUCUNE,0}}; if(sur_plateau(m.tr,m.tc)&&(plateau[m.tr][m.tc].type==VIDE||plateau[m.tr][m.tc].col!=side)&&coup_legal(m,side)) if(count<max)moves[count++]=m;}}
        else if(p.type==FOU||p.type==TOUR||p.type==DAME){
            int dirs[8][2],dlim=0; if(p.type==FOU){int tmp[4][2]={{1,1},{1,-1},{-1,1},{-1,-1}}; for(dlim=0;dlim<4;dlim++){dirs[dlim][0]=tmp[dlim][0];dirs[dlim][1]=tmp[dlim][1];}} else if(p.type==TOUR){int tmp[4][2]={{1,0},{-1,0},{0,1},{0,-1}}; for(dlim=0;dlim<4;dlim++){dirs[dlim][0]=tmp[dlim][0];dirs[dlim][1]=tmp[dlim][1];}} else{int tmp[8][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}}; for(dlim=0;dlim<8;dlim++){dirs[dlim][0]=tmp[dlim][0];dirs[dlim][1]=tmp[dlim][1];}}
            for(int d=0;d<dlim;d++){int rr=r+dirs[d][0],cc=c+dirs[d][1]; while(sur_plateau(rr,cc)){Coup m={r,c,rr,cc,{VIDE,AUCUNE,0}}; if(plateau[rr][cc].type==VIDE){if(coup_legal(m,side)) if(count<max)moves[count++]=m;} else {if(plateau[rr][cc].col!=side){if(coup_legal(m,side)) if(count<max)moves[count++]=m;} break;} rr+=dirs[d][0]; cc+=dirs[d][1];}}
        } else if(p.type==ROI){
            for(int dr=-1;dr<=1;dr++) for(int dc=-1;dc<=1;dc++){if(dr==0&&dc==0)continue; Coup m={r,c,r+dr,c+dc,{VIDE,AUCUNE,0}}; if(sur_plateau(m.tr,m.tc)&&(plateau[m.tr][m.tc].type==VIDE||plateau[m.tr][m.tc].col!=side)&&coup_legal(m,side)) if(count<max)moves[count++]=m;}
            // proposer roque si le roi n'a pas bougé et a verifier dans coup_legal si les conditions sont remplies
            if(!p.aBouge){
                Coup kside={r,c,r,c+2,{VIDE,AUCUNE,0}}; if(sur_plateau(kside.tr,kside.tc) && coup_legal(kside,side) && count<max) moves[count++]=kside;
                Coup qside={r,c,r,c-2,{VIDE,AUCUNE,0}}; if(sur_plateau(qside.tr,qside.tc) && coup_legal(qside,side) && count<max) moves[count++]=qside;
            }
        }
    }
    return count;
}

// échec et mat
int est_echec_et_mat(Couleur side){
    if(!est_en_echec(side)) return 0;
    Coup coups[1024]; int n=gen_tous(side,coups,1024);
    for(i=0;i<n;i++) if(coup_legal(coups[i],side)) return 0;
    return 1;
}

// parse coup
int parse_coup(const char *s,Coup *m){
    // nettoyer: retirer espaces et normaliser lettres majuscules en minuscules
    char tmp[64]; int ti=0; for(i=0;s[i] && ti<63;i++){
        char ch = s[i];
        if(ch==' '||ch=='\t') continue;
        // convertir A-Z -> a-z methode du cours d'UNIX
        if(ch>='A' && ch<='Z') ch = ch - 'A' + 'a';
        tmp[ti++] = ch;
    }
    tmp[ti]=0;
    int len = ti;
    if(len==0) return 0;
    // si l'entrée est un nombre (sélection dans la liste suggérée)
    int alldigits=1; for(i=0;i<len;i++){ if(tmp[i]<'0'||tmp[i]>'9') { alldigits=0; break; } }
    if(alldigits){
        // convertir vers int
        int val=0; for(i=0;i<len;i++) val = val*10 + (tmp[i]-'0');
        if(val>=1 && val<=last_moves_n){ *m = last_moves[val-1]; return 1; }
        return 0;
    }
    // format attendu: e2e4 or e7e8q (promo)
    if(len<4) return 0;
    char f1=tmp[0], r1=tmp[1], f2=tmp[2], r2=tmp[3];
    if(f1<'a'||f1>'h'||f2<'a'||f2>'h'||r1<'1'||r1>'8'||r2<'1'||r2>'8') return 0;
    m->fr=rang_lig(r1); m->fc=fichier_col(f1); m->tr=rang_lig(r2); m->tc=fichier_col(f2);
    m->promo.type=VIDE; m->promo.col=AUCUNE;
    if(len>=5){ char p=tmp[4]; if(p=='d'){ m->promo.type=DAME; m->promo.col=trait; } else if(p=='t'){ m->promo.type=TOUR; m->promo.col=trait; } else if(p=='f'){ m->promo.type=FOU; m->promo.col=trait; } else if(p=='c'){ m->promo.type=CAVALIER; m->promo.col=trait; } }
    return 1;
}
// suggérer coups possibles pour mettre un bot plus tard
// remplit aussi last_moves[] pour permettre la sélection par numéro
void suggerer() {
    Coup coups[1024];
    int n = gen_tous(trait, coups, 1024);
    last_moves_n = 0;
    // aucun coup légal
    if (n == 0) {
        printf("Aucun coup légal.\n");
        return;
    }
    printf("\nPossibilites:\n");

    for (int i = 0; i < n; i++) {
        // stocker et eviter depassement de last_moves[]
        if (last_moves_n < 1024) last_moves[last_moves_n++] = coups[i];
        // formatage du coup type e2e4, e7e8d etc.
        char f = 'a' + coups[i].fc;
        char t = 'a' + coups[i].tc;
        int fr = 8 - coups[i].fr;
        int tr = 8 - coups[i].tr;
        char promo = 0;
        if (coups[i].promo.type != VIDE) {
            switch (coups[i].promo.type) {
                case TOUR:     promo = 't'; break;
                case FOU:      promo = 'f'; break;
                case CAVALIER: promo = 'c'; break;
                default:       promo = 'd'; break; // dame par défaut
            }
        }

        // affichage e2e4 ou e7e8d etc.
        // f = fichier, fr = rangée de départ, t = fichier de destination, tr = rangée de destination
        // promo = caractère de promotion si applicable sinon 0
        if (promo)
            printf(" %2d:%c%d%c%d%c", i + 1, f, fr, t, tr, promo);
        else
            printf(" %2d:%c%d%c%d",  i + 1, f, fr, t, tr);

        // retour ligne toutes les 4 colonnes
        if ((i + 1) % 4 == 0)
            printf("\n");
        else
            printf("  ");
    }
    // fin de ligne si n % 4 != 0
    if (n % 4 != 0) printf("\n");
    printf("\n");
}

int main(){
    // principale
    // coup m = {from_row, from_col, to_row, to_col, promotion_piece}
    init_plateau(); char input[128]; Coup m;
    printf("'help' pour l'aide.\n"); afficher();
    while(1){
        // suggérer coups possibles pour enlever les problèmes de calcul sur last_moves[]
        suggerer();
        printf("%s: ",trait==BLANC?"Blanc":"Noir"); if(!fgets(input,sizeof(input),stdin)) break;
        // nettoyer l'entrée
        int L=strlen(input); if(L>0&&input[L-1]=='\n') input[L-1]=0;
        // strcmp vient de string.h
        if(strcmp(input,"quit")==0) break;
        if(strcmp(input,"resign")==0){printf("\n\n\n> %s abandonne. Gagnant: %s\n\n",trait==BLANC?"Blanc":"Noir", trait==BLANC?"Noir":"Blanc"); break;}
        if(strcmp(input,"afficher")==0){
            printf("\n> Affichage:\n");
            afficher();
            continue;
        }
        if(strcmp(input,"help")==0){
            printf(
                "\n\n> Aide :\n"
                "     afficher : afficher le plateau\n"
                "     quit    : quitter\n"
                "     resign  : abandonner\n"
                "     help    : afficher l'aide\n"
                "     Coups   : e2e4 (origine puis destination)\n"
                "     Option  : option d=Dame, t=Tour, f=Fou, c=Cavalier\n"
            );
            continue;
        }
        // A developper pourquoi pas bon
        if(!parse_coup(input,&m)||!coup_legal(m,trait)){printf("\n\n\n\n> Coup interdit: %s\n",input); continue;}
        // montrer le coup
        appliquer(m); afficher();
        // vérif échec et mat
        if(est_echec_et_mat(trait==BLANC?NOIR:BLANC)){printf("\n\n\n> Échec et mat! %s gagne.\n\n",trait==BLANC?"Blanc":"Noir"); break;}
        // changer de trait
        trait=(trait==BLANC?NOIR:BLANC);
    }
    return 0;
}
// met rien apres main ca boeug sais pas pourquoi

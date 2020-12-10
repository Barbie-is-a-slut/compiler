#include    "CodeGenerator.h"


/*  ###############################################################  */
//STRUCTS DEFINITIONS

//SYMBOL TABLE STRUCT
typedef struct symbol_table {
    struct variable *var;
    struct symbol_table *next;
} *Symbol_table;

//DIMENSIONS STRUCT TO HOLD (ARRAY) DIMENSIONS
typedef struct dimensions {
    int size;
    int index;
    int *vals;
} *Dimensions;

//VARIABLE STRUCT
typedef struct variable {
    char *name;
    int type;
    int size;
    int address;
    int nested_lvl;
    Dimensions dims;
    Symbol_table st;
} *Variable;

/*  ###############################################################  */


/*  ###############################################################  */
//GLOBAL VARIABLES AND OTHER JUNK

//DEFINES
#define BOOL int
#define TRUE 1
#define FALSE 0

//PREVIOUS STUFF
int Nested_lvl = 0;
int Addr = 5;
int Type = INT;
BOOL DECL = FALSE;
Symbol_table ST = NULL;

//POINTERS STRUCTS ARRAYS SYUFF
int star_num = 0;
BOOL ARRAY_DECL = FALSE;
BOOL sel = FALSE;
int struct_addr_shift = 0;
BOOL STRUCT_DEF;
BOOL REFR = FALSE;
Symbol_table struct_ST = NULL;
Symbol_table GST = NULL;

//LABELS STUFF
int if_label = 0;
int ifelse_label = 0;
int while_label = 0;
int for_label = 0;
int cond_label = 0;
int do_while_label = 0;
int switch_label = 0;
int case_num = 0;
char *BREAK_TYPE[4] = {"while_end", "for_end", "do_while_end", "switch_end"};
int BREAK_ind = 0;
int BREAK_LABRL_NUM = 0;

/*  ###############################################################  */


/*  ###############################################################  */
//FUNCTIONS FOR "struct dimensions"

//CREATE DIMENSION .ZERO DIMENSIONS TO BEGIN WITH
Dimensions create_dims() {
    Dimensions d = malloc(sizeof(*d));
    if (!d)
        return NULL;
    d->vals = malloc(sizeof(int) * 5);
    if (!d->vals) {
        free(d);
        return NULL;
    }
    d->size = 5;
    d->index = 0;
    return d;
}

//FREE DIMENSIONS
void delete_dims(Dimensions dims) {
    if (!dims)
        return;
    free(dims->vals);
    free(dims);
}

//DOUBLE THE SIZE OF DIMS (DYNAMIC ARRAY)
Dimensions dim_realloc(Dimensions dims) {
    if (!dims)
        return NULL;
    int *tmp = malloc(sizeof(int) * dims->size * 2);
    if (!tmp)
        return NULL;
    for (int i = 0; i < dims->index; i++)
        tmp[i] = dims->vals[i];
    free(dims->vals);
    dims->vals = tmp;
    return dims;
}

//ADD 1 D AT INDEX
Dimensions add_1_dime(Dimensions dims, int d) {
    if (dims && dims->index == dims->size)
        dims = dim_realloc(dims);
    if (!dims)
        return dims;
    dims->vals[dims->index++] = d;
    return dims;
}

//MUL N VALUES OF DIMS (N-1 IN REALITY) STARTING FROM THE END.TO INCLUDE FIRST D PASS N=#DIMENSIONS+1
int mul_N_dime(Dimensions dims, int N) {
    int mul = 1;
    for (int i = dims->index - 1; i > dims->index - 1 - N; i--)
        mul *= dims->vals[i];
    return mul;
}

/*  ###############################################################  */


/*  ###############################################################  */
//FUNCTIONS FOR "struct variable"

//CREATE VARIABLE
Variable create_var(char *name, int type, int size, int address, int nested_lvl, Dimensions dims, Symbol_table st) {
    Variable var = malloc(sizeof(*var));
    var->name = malloc((1 + strlen(name)) * sizeof(char));
    if (!var->name)
        return NULL;
    strcpy(var->name, name);
    var->type = type;
    var->size = size;
    var->address = address;
    var->nested_lvl = nested_lvl;
    var->dims = dims;
    var->st = st;
    return var;
}

//FREE VARIABLE
void free_var(Variable var) {
    if (!var)
        return;
    delete_dims(var->dims);
    free(var->name);
    free(var);
}
/*  ###############################################################  */


/*  ###############################################################  */
//FUNCTIONS FOR "struct symbol_table"

//CREATE SYMBOL TABLE
Symbol_table create_st(Variable var) {
    Symbol_table st = malloc(sizeof(*st));
    st->var = var;
    st->next = NULL;
    return st;
}

//FREE SYMBOL TABLE
void free_st(Symbol_table st) {
    if (!st)
        return;
    free_st(st->next);
    free_var(st->var);
    free(st);
}

//ADD VARIABLE TO THE END OF THE SYMBOL TABLE ,COULD BE USED FOR PRINTING VARS IN ST (LINES 193,198,202,206)
// LEFT THESE PRINTS ON ,HELPFUL FOR UNDERSTANDING HOW THE STs LOOk
Symbol_table add_last_st(Symbol_table st, Variable var) {
    if (!st)
        return NULL;
    if (!st->var) {
        printf("%s %d (%d-%d-%d)\n", var->name, var->address, var->type, var->st != NULL, var->size);
        st->var = var;
        return st;
    }
    for (Symbol_table cur = st; cur; cur = cur->next) {
        printf("%s %d (%d-%d-%d) ", cur->var->name, cur->var->address, cur->var->type, cur->var->st != NULL,
               cur->var->size);
        if (!cur->next) {
            printf("%s %d (%d-%d-%d) ", var->name, var->address, var->type, var->st != NULL, var->size);
            cur->next = create_st(var);
            break;
        }
    }
    printf("\n");
    return st;
}

//REMOVE ALL  VARIABLES AT NESTED LEVEL = lvl (LAST LEVEL) + UPDATE GLOABAL ADDR
Symbol_table remove_last_lvl_st(Symbol_table st, int lvl) {
    if (!st || !st->var)
        return st;
    Addr = 5;
    if (st->var->nested_lvl == lvl) {
        free_var(st->var);
        free_st(st->next);
        return st;
    }
    for (Symbol_table cur = st; cur && cur->next; cur = cur->next) {
        Addr += cur->var->size;
        if (cur->next->var->nested_lvl == lvl) {
            free_st(cur->next);
            cur->next = NULL;
        }
    }
    return st;
}

//FIND THE LAST VARIABLE WITH MATCHING NAME
Variable find_var_st(Symbol_table st, char *name) {
    Variable var = NULL;
    for (Symbol_table cur = st; cur; cur = cur->next)
        if (cur->var && strcmp(cur->var->name, name) == 0)
            var = cur->var;

    return var;
}

//RETURN VARIABLE AT ADDRESS=addr IN ST .POSSIABLE REDUNDANT CASES (APPLIES FOR ALL THE CODE :) )
Variable find_addr_var(Symbol_table st, int addr) {
    for (Symbol_table cur = st; cur; cur = cur->next) {
        //printf("name:%s-add:%d\n",cur->var->name,cur->var->address);
        if (cur->var && cur->var->address == addr)
            return cur->var;
        if (cur->var && cur->var->st && addr > 4) {
            Variable var = find_addr_var(cur->var->st, addr - cur->var->address);
            if (var)
                return var;
        }
        if (cur->var && addr > cur->var->address && addr < cur->var->address + cur->var->size)
            return cur->var;
    }
    return NULL;
}

//RETURN THE LAST VARIABLE IN ST
Variable get_last_var(Symbol_table st) {
    Variable var = NULL;
    for (Symbol_table cur = st; cur; cur = cur->next)
        var = cur->var;
    return var;
}

/*  ###############################################################  */


/*  ###############################################################  */
//NEW CODE_RECUR_AUX TO ENABLE FLAG AS PARAMETER
//CALLED BY THE ORIGINAL CODE_RECUR
/*
*	This recursive function is the main method for Code Generation
*	Input: treenode (AST)
*	flag: FALSE => VALUE ,TRUE => ADDRESS
*	Output: prints the Pcode on the console
*/
int code_recur_aux(treenode *root, BOOL flag, int cur_switch, Variable **OPS, Variable *OPARR, int ixa_lvl) {
    if_node *ifn;
    for_node *forn;
    leafnode *leaf;
    int prev_BREAK_ind = BREAK_ind;
    int prev_BREAK_LABRL_NUM = BREAK_LABRL_NUM;
    Symbol_table prev_sT = ST;
    if (!root)
        return SUCCESS;
    switch (root->hdr.which) {
        case LEAF_T:
            leaf = (leafnode *) root;
            switch (leaf->hdr.type) {
                case TN_LABEL:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_IDENT:
                    /* variable case */
                    /*
                    *	In order to get the identifier name you have to use:
                    *	leaf->data.sval->str
                    */
                    if (strcmp(leaf->data.sval->str, "main") == 0)
                        break;
                    Variable v = find_var_st(ST, leaf->data.sval->str);

                    if (!sel && (!v || DECL == TRUE)) {
                        if (REFR) {
                            Type = v->address;
                            break;
                        }
                        Variable var = create_var(leaf->data.sval->str, Type, 1, Addr, Nested_lvl, NULL, NULL);
                        if (ARRAY_DECL) {
                            var->dims = create_dims();
                        }
                        if (Type == STRUCT && STRUCT_DEF) {
                            var->st = create_st(NULL);
                        }
                        if (Type < 0) {
                            Variable str = find_addr_var(ST, Type);
                            var->st = str->st;
                            if (!star_num)
                                var->size = str->size;
                        }
                        Addr = Addr + var->size;
                        if (struct_ST) {
                            Variable tmp = get_last_var(struct_ST);
                            if (tmp) {
                                var->address = tmp->address + tmp->size;
                            }
                            add_last_st(struct_ST, var);
                        } else add_last_st(ST, var);
                    } else {
                        if (sel)
                            printf("inc %d\n", v->address);
                        else printf("ldc %d\n", v->address);
                        if (flag == FALSE)
                            printf("ind\n");
                        if (*OPS)
                            **OPS = v->type < 0 ? find_addr_var(ST, v->type) : NULL;
                        if (OPARR)
                            *OPARR = v;
                        return v->address; //alt. define return res.
                    }

                    break;

                case TN_COMMENT:
                    /* Maybe you will use it later */
                    break;

                case TN_ELLIPSIS:
                    /* Maybe you will use it later */
                    break;

                case TN_STRING:
                    /* Maybe you will use it later */
                    break;

                    char *TypeName;
                case TN_TYPE:
                    /* Maybe you will use it later */
                    Type = leaf->hdr.tok;
                    break;

                case TN_INT:
                    /* Constant case */
                    /*
                    *	In order to get the int value you have to use:
                    *	leaf->data.ival
                    */
                    if (ARRAY_DECL) {
                        Variable arr = STRUCT_DEF ? get_last_var(struct_ST) : get_last_var(ST);
                        add_1_dime(arr->dims, leaf->data.ival);
                        arr->size *= leaf->data.ival;
                        Addr = arr->address + arr->size;
                    } else printf("ldc %d\n", leaf->data.ival);
                    break;

                case TN_REAL:
                    /* Constant case */
                    /*
                    *	In order to get the real value you have to use:
                    *	leaf->data.dval
                    */
                    printf("ldc %lf\n", leaf->data.dval);
                    break;
            }
            break;

        case IF_T:
            ifn = (if_node *) root;
            switch (ifn->hdr.type) {
                case TN_IF:
                    if (ifn->else_n == NULL) {
                        /* if case (without else)*/
                        int if_num = if_label++;
                        code_recur_aux(ifn->cond, FALSE, cur_switch, OPS, OPARR, 0);
                        printf("fjp if_end%d\n", if_num);
                        code_recur_aux(ifn->then_n, FALSE, cur_switch, OPS, OPARR, 0);
                        printf("if_end%d:\n", if_num);
                    } else {
                        /* if - else case*/
                        int ifelse_num = ifelse_label++;
                        code_recur_aux(ifn->cond, FALSE, cur_switch, OPS, OPARR, 0);
                        printf("fjp ifelse_else%d\n", ifelse_num);
                        code_recur_aux(ifn->then_n, FALSE, cur_switch, OPS, OPARR, 0);
                        printf("ujp ifelse_end%d\n", ifelse_num);
                        printf("ifelse_else%d:\n", ifelse_num);
                        code_recur_aux(ifn->else_n, FALSE, cur_switch, OPS, OPARR, 0);
                        printf("ifelse_end%d:\n", ifelse_num);
                    }
                    break;
                    int cond_num;
                case TN_COND_EXPR:
                    /* (cond)?(exp):(exp); */
                    cond_num = cond_label++;
                    code_recur_aux(ifn->cond, FALSE, cur_switch, OPS, OPARR, 0);
                    printf("fjp cond_else%d\n", cond_num);
                    code_recur_aux(ifn->then_n, FALSE, cur_switch, OPS, OPARR, 0);
                    printf("ujp condLabel_end%d\n", cond_num);
                    printf("cond_else%d:\n", cond_num);
                    code_recur_aux(ifn->else_n, FALSE, cur_switch, OPS, OPARR, 0);
                    printf("condLabel_end%d:\n", cond_num);
                    break;

                default:
                    /* Maybe you will use it later */
                    code_recur_aux(ifn->cond, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(ifn->then_n, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(ifn->else_n, FALSE, cur_switch, OPS, OPARR, 0);
            }
            break;

        case FOR_T:
            forn = (for_node *) root;
            switch (forn->hdr.type) {

                case TN_FUNC_DEF:
                    /* Function definition */
                    /* e.g. int main(...) { ... } */
                    /* Look at the output AST structure! */
                    code_recur_aux(forn->init, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(forn->test, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(forn->incr, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(forn->stemnt, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                    int for_num;
                case TN_FOR:
                    /* For case*/
                    /* e.g. for(i=0;i<5;i++) { ... } */
                    /* Look at the output AST structure! */

                    for_num = for_label++;
                    BREAK_ind = 1;
                    BREAK_LABRL_NUM = for_num;
                    code_recur_aux(forn->init, FALSE, cur_switch, OPS, OPARR, 0);
                    printf("for_loop%d:\n", for_num);
                    code_recur_aux(forn->test, FALSE, cur_switch, OPS, OPARR, 0);
                    printf("fjp for_end%d\n", for_num);
                    code_recur_aux(forn->stemnt, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(forn->incr, FALSE, cur_switch, OPS, OPARR, 0);
                    printf("ujp for_loop%d\n", for_num);
                    printf("for_end%d:\n", for_num);
                    break;

                default:
                    /* Maybe you will use it later */
                    code_recur_aux(forn->init, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(forn->test, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(forn->stemnt, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(forn->incr, FALSE, cur_switch, OPS, OPARR, 0);
            }
            break;

        case NODE_T:
            switch (root->hdr.type) {
                case TN_PARBLOCK:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_PARBLOCK_EMPTY:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_TRANS_LIST:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_FUNC_DECL:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_FUNC_CALL:
                    /* Function call */
                    if (strcmp(((leafnode *) root->lnode)->data.sval->str, "printf") == 0) {
                        /* printf case */
                        /* The expression that you need to print is located in */
                        /* the currentNode->right->right sub tree */
                        /* Look at the output AST structure! */
                        code_recur_aux(root->rnode->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                        printf("print\n");
                    } else {
                        /* other function calls - for HW3 */
                        code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                        code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    }
                    break;

                case TN_BLOCK:
                    /* Maybe you will use it later */
                    Nested_lvl++;
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    remove_last_lvl_st(ST, Nested_lvl);
                    Nested_lvl--;
                    break;

                    BOOL pAD = ARRAY_DECL;
                case TN_ARRAY_DECL:
                    /* array declaration - for HW2 */

                    pAD = ARRAY_DECL;
                    ARRAY_DECL = TRUE;
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    ARRAY_DECL = pAD;
                    break;

                case TN_EXPR_LIST:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_NAME_LIST:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_ENUM_LIST:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_FIELD_LIST:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_PARAM_LIST:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_IDENT_LIST:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_TYPE_LIST:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_COMP_DECL:
                    /* struct component declaration - for HW2 */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;


                case TN_DECL:
                    /* structs declaration - for HW2 */
                    DECL = DECL;
                    int prev_DECL = DECL;
                    DECL = TRUE;
                    star_num = 0;
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    star_num = 0;
                    DECL = prev_DECL;
                    break;

                case TN_DECL_LIST:
                    /* Maybe you will use it later */

                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_DECLS:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_STEMNT_LIST:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_STEMNT:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_BIT_FIELD:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_PNTR:
                    /* pointer - for HW2! */
                    star_num++;
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_TYPE_NME:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_INIT_LIST:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_INIT_BLK:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                    int prevAddr;
                case TN_OBJ_DEF:
                    /* Maybe you will use it later */

                    prevAddr = Addr;
                    Addr = -1;
                    Type = STRUCT;
                    int pDEF = STRUCT_DEF;
                    STRUCT_DEF = TRUE;
                    Symbol_table pST = ST;
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    Variable s = get_last_var(ST);
                    if (!s->st)
                        s->st = create_st(NULL);
                    Symbol_table psst = struct_ST;
                    struct_ST = s->st;
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    struct_ST = psst;
                    s->size = Addr;
                    s->address -= struct_addr_shift++;
                    Addr = prevAddr;
                    STRUCT_DEF = pDEF;
                    ST = pST;
                    break;

                    BOOL pREFR;
                case TN_OBJ_REF:
                    /* Maybe you will use it later */
                    pREFR = REFR;
                    REFR = TRUE;
                    Symbol_table prST = ST;
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    REFR = pREFR;
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    ST = prST;
                    break;

                case TN_CAST:
                    /* Maybe you will use it later */
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    break;

                case TN_JUMP:
                    if (root->hdr.tok == RETURN) {
                        /* return jump - for HW2! */
                        code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                        code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    } else if (root->hdr.tok == BREAK) {
                        /* break jump - for HW2! */
                        printf("ujp %s%d\n", BREAK_TYPE[BREAK_ind], BREAK_LABRL_NUM);
                        code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                        code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    } else if (root->hdr.tok == GOTO) {
                        /* GOTO jump - for HW2! */
                        code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                        code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    }
                    break;
                    int switch_label_num, prev_case;
                case TN_SWITCH:
                    /* Switch case - for HW2! */
                    prev_case = case_num;
                    case_num = 0;
                    switch_label_num = switch_label++;
                    BREAK_ind = 3;
                    BREAK_LABRL_NUM = switch_label_num;
                    code_recur_aux(root->lnode, FALSE, switch_label_num, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, switch_label_num, OPS, OPARR, 0);
                    printf("switch%d_case%d:\n", switch_label_num, case_num);
                    printf("switch_end%d:\n", switch_label_num);
                    case_num = prev_case;
                    break;

                    Variable *pOPARR;
                case TN_INDEX:
                    /* call for array - for HW2! */
                    pOPARR = OPARR;
                    if (OPARR == NULL) {
                        OPARR = malloc(sizeof(Variable));
                        *OPARR = NULL;
                    }
                    int arr_addr = code_recur_aux(root->lnode, TRUE, cur_switch, OPS, OPARR, ixa_lvl+1);
                    Variable var = *OPARR;
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, NULL, 0);

                    printf("ixa %d\n", (var->size / mul_N_dime(var->dims, var->dims->index)) *
                                       mul_N_dime(var->dims, ixa_lvl));
                    if (ixa_lvl == 0)
                        printf("dec 0\n");
                    if (!flag)
                        printf("ind\n");
                    if (!pOPARR)
                        free(OPARR);
                    OPARR = pOPARR;
                    return arr_addr;
                    break;

                case TN_DEREF:
                    /* pointer derefrence - for HW2! */
                    code_recur_aux(root->lnode, flag, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, flag, cur_switch, OPS, OPARR, 0);
                    printf("ind\n");
                    break;

                case TN_SELECT:
                    /* struct case - for HW2! */
                    if (root->hdr.tok == ARROW) {
                        /* Struct select case "->" */
                        /* e.g. struct_variable->x; */
                        code_recur_aux(root->lnode, TRUE, cur_switch, OPS, OPARR, 0);
                        code_recur_aux(root->rnode, flag, cur_switch, OPS, OPARR, 0);
                    } else {
                        /* Struct select case "." */
                        /* e.g. struct_variable.x; */
                        Symbol_table prevST = ST;
                        BOOL pSel = sel;
                        Variable *pOPS = *OPS;
                        *OPS = malloc(sizeof(Variable));
                        int struct_addr = code_recur_aux(root->lnode, TRUE, cur_switch, OPS, OPARR, 0);
                        ST = (**OPS)->st;
                        sel = TRUE;
                        int field_addr = code_recur_aux(root->rnode, flag, cur_switch, OPS, OPARR, 0);
                        Variable tmp = find_addr_var(ST, field_addr);
                        ST = prevST;
                        *OPS = pOPS;
                        if(*OPS) {
                            if (tmp->type == STRUCT)
                                **OPS = tmp;
                            else if (tmp->type < 0)
                                **OPS = find_addr_var(GST, tmp->type);
                        }
                        sel = pSel;
                        return struct_addr;
                    }
                    break;

                case TN_ASSIGN:
                    if (root->hdr.tok == EQ) {
                        /* Regular assignment "=" */
                        /* e.g. x = 5; */
                        code_recur_aux(root->lnode, TRUE, cur_switch, OPS, OPARR, 0);
                        code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                        printf("sto\n");
                    } else if (root->hdr.tok == PLUS_EQ) {
                        /* Plus equal assignment "+=" */
                        /* e.g. x += 5; */
                        code_recur_aux(root->lnode, TRUE, cur_switch, OPS, OPARR, 0);
                        code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                        code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                        printf("add\n");
                        printf("sto\n");
                    } else if (root->hdr.tok == MINUS_EQ) {
                        /* Minus equal assignment "-=" */
                        /* e.g. x -= 5; */
                        code_recur_aux(root->lnode, TRUE, cur_switch, OPS, OPARR, 0);
                        code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                        code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                        printf("sub\n");
                        printf("sto\n");
                    } else if (root->hdr.tok == STAR_EQ) {
                        /* Multiply equal assignment "*=" */
                        /* e.g. x *= 5; */
                        code_recur_aux(root->lnode, TRUE, cur_switch, OPS, OPARR, 0);
                        code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                        code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                        printf("mul\n");
                        printf("sto\n");
                    } else if (root->hdr.tok == DIV_EQ) {
                        /* Divide equal assignment "/=" */
                        /* e.g. x /= 5; */
                        code_recur_aux(root->lnode, TRUE, cur_switch, OPS, OPARR, 0);
                        code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                        code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                        printf("div\n");
                        printf("sto\n");
                    }
                    break;

                case TN_EXPR:
                    switch (root->hdr.tok) {
                        case B_AND:
                            /* address token "&" */
                            /* e.g. &x; */
                            code_recur_aux(root->lnode, TRUE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, TRUE, cur_switch, OPS, OPARR, 0);
                            break;

                        case CASE:
                            code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            break;

                        case INCR:
                            /* Increment token "++" */
                            if (!root->rnode) {
                                code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                                code_recur_aux(root->lnode, TRUE, cur_switch, OPS, OPARR, 0);
                                code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                                printf("inc 1\n");
                                printf("sto\n");
                            } else {
                                code_recur_aux(root->rnode, TRUE, cur_switch, OPS, OPARR, 0);
                                code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                                printf("inc 1\n");
                                printf("sto\n");
                                code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            }

                            break;

                        case DECR:
                            /* Decrement token "--" */
                            if (!root->rnode) {
                                code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                                code_recur_aux(root->lnode, TRUE, cur_switch, OPS, OPARR, 0);
                                code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                                printf("dec 1\n");
                                printf("sto\n");
                            } else {
                                code_recur_aux(root->rnode, TRUE, cur_switch, OPS, OPARR, 0);
                                code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                                printf("dec 1\n");
                                printf("sto\n");
                                code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            }
                            break;

                        case PLUS:
                            /* Plus token "+" */
                            code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            printf("add\n");
                            break;

                        case MINUS:
                            /* Minus token "-" */
                            code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            if (!root->lnode)
                                printf("neg\n");
                            else printf("sub\n");
                            break;

                        case DIV:
                            /* Divide token "/" */
                            code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            printf("div\n");
                            break;

                        case STAR:
                            /* multiply token "*" */
                            code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            printf("mul\n");
                            break;

                        case AND:
                            /* And token "&&" */
                            code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            printf("and\n");
                            break;

                        case OR:
                            /* Or token "||" */
                            code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            printf("or\n");
                            break;

                        case NOT:
                            /* Not token "!" */
                            code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            printf("not\n");
                            break;

                        case GRTR:
                            /* Greater token ">" */
                            code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            printf("grt\n");
                            break;

                        case LESS:
                            /* Less token "<" */
                            code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            printf("les\n");
                            break;

                        case EQUAL:
                            /* Equal token "==" */
                            code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            printf("equ\n");
                            break;

                        case NOT_EQ:
                            /* Not equal token "!=" */
                            code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            printf("neq\n");
                            break;

                        case LESS_EQ:
                            /* Less or equal token "<=" */
                            code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            printf("leq\n");
                            break;

                        case GRTR_EQ:
                            /* Greater or equal token ">=" */
                            code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            printf("geq\n");
                            break;

                        default:
                            code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                            code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                            break;
                    }
                    break;

                    int while_num;
                case TN_WHILE:
                    /* While case */
                    while_num = while_label++;
                    BREAK_ind = 0;
                    BREAK_LABRL_NUM = while_num;
                    printf("while_loop%d:\n", while_num);
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    printf("fjp while_end%d\n", while_num);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    printf("ujp while_loop%d\n", while_num);
                    printf("while_end%d:\n", while_num);
                    break;

                    int do_while_num;
                case TN_DOWHILE:
                    /* Do-While case */
                    do_while_num = do_while_label++;
                    BREAK_ind = 2;
                    BREAK_LABRL_NUM = do_while_num;
                    printf("do_while_loop%d:\n", do_while_num);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    printf("fjp do_while_end%d\n", do_while_num);
                    printf("ujp do_while_loop%d\n", do_while_num);
                    printf("do_while_end%d:\n", do_while_num);
                    break;


                    int local_case_num;
                case TN_LABEL:
                    /* Maybe you will use it later */
                    local_case_num = case_num++;
                    printf("switch%d_case%d:\n", cur_switch, local_case_num);
                    printf("dpl\n");
                    code_recur_aux(root->lnode, flag, cur_switch, OPS, OPARR, 0);
                    printf("equ\n");
                    printf("fjp switch%d_case%d\n", cur_switch, local_case_num + 1);
                    case_num = ++local_case_num;
                    code_recur_aux(root->rnode, flag, cur_switch, OPS, OPARR, 0);
                    break;

                default:
                    code_recur_aux(root->lnode, FALSE, cur_switch, OPS, OPARR, 0);
                    code_recur_aux(root->rnode, FALSE, cur_switch, OPS, OPARR, 0);
                case TN_EMPTY:
                    break;
                case TN_FUNC_DEF:
                    break;
                case TN_IF:
                    break;
                case TN_FOR:
                    break;
                case TN_ADDROF:
                    break;
                case TN_COND_EXPR:
                    break;
                case TN_COMMENT:
                    break;
                case TN_CPP:
                    break;
                case TN_ELLIPSIS:
                    break;
                case TN_IDENT:
                    break;
                case TN_TYPE:
                    break;
                case TN_STRING:
                    break;
                case TN_INT:
                    break;
                case TN_REAL:
                    break;
                case TN_PARFOR:
                    break;
            }
            break;

        case NONE_T:
            printf("Error: Unknown node type!\n");
            exit(FAILURE);
    }
    ST = prev_sT;
    BREAK_ind = prev_BREAK_ind;
    BREAK_LABRL_NUM = prev_BREAK_LABRL_NUM;
    return SUCCESS;
}

int code_recur(treenode *root) {
    ST = create_st(NULL);
    GST = ST; //TO KEEP TRACK OF THE GLOBAL SYMBOL TABLE (ST)
    Variable **OPS = malloc(sizeof(*OPS));
    return code_recur_aux(root, FALSE, 0, OPS, NULL, 0);
    free_st(ST);
}

/*
*	This function prints all the variables on your symbol table with their data
*	Input: treenode (AST)
*	Output: prints the Sumbol Table on the console
*/
void print_symbol_table(treenode *root) {
    printf("---------------------------------------\n");
    printf("Showing the Symbol Table:\n");
    /*
    *	add your code here
    */
}


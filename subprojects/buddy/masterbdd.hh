#pragma once
#include "kernel.hh"
#include "types.hh"
#include "imatrix.hh"

class masterbdd {
  public:
    /************************************************************************* kernel */

    /*
     NAME    {* bddtrue *}
     SECTION {* kernel *}
     SHORT   {* the constant true bdd *}
     PROTO   {* const BDD bddtrue; *}
     DESCR   {* This bdd holds the constant true value *}
     ALSO    {* bddfalse, bdd\_true, bdd\_false *}
     */
    const BDD bddtrue=1;                     /* The constant true bdd */

    /*
     NAME    {* bddfalse*}
     SECTION {* kernel *}
     SHORT   {* the constant false bdd *}
     PROTO   {* const BDD bddfalse; *}
     DESCR   {* This bdd holds the constant false value *}
     ALSO    {* bddtrue, bdd\_true, bdd\_false *}
     */
    const BDD bddfalse=0;                    /* The constant false bdd */

    bddinthandler  bdd_error_hook(bddinthandler);
    bddgbchandler  bdd_gbc_hook(bddgbchandler);
    bdd2inthandler bdd_resize_hook(bdd2inthandler);
    bddinthandler  bdd_reorder_hook(bddinthandler);
    bddfilehandler bdd_file_hook(bddfilehandler);

    int      bdd_init(int, int);
    void     bdd_done(void);
    int      bdd_setvarnum(int);
    int      bdd_extvarnum(int);
    int      bdd_isrunning(void) const;
    int      bdd_setmaxnodenum(int);
    int      bdd_setmaxincrease(int);
    int      bdd_setminfreenodes(int);
    int      bdd_getnodenum(void);
    int      bdd_getallocnum(void) const;
    char*    bdd_versionstr(void) const;
    int      bdd_versionnum(void) const;
    void     bdd_stats(bddStat *);
    void     bdd_cachestats(bddCacheStat *);
    void     bdd_fprintstat(FILE *);
    void     bdd_printstat(void);
    static void     bdd_default_gbchandler(int, bddGbcStat *);
    static void     bdd_default_errhandler(int);
    static const char *bdd_errstring(int);
    BDD bdd_true (void) const;
    BDD bdd_false (void) const;
    void     bdd_clear_error(void);
    int      bdd_varnum(void) const;
    BDD      bdd_ithvar(int) const;
    BDD      bdd_nithvar(int) const;
    int      bdd_var(BDD) const;
    BDD      bdd_low(BDD) const;
    BDD      bdd_high(BDD) const;
    int      bdd_varlevel(int) const;
    BDD      bdd_addref_nc(BDD);
    BDD      bdd_addref(BDD);
    BDD      bdd_delref_nc(BDD);
    BDD      bdd_delref(BDD);
    void     bdd_gbc(void);
    int      bdd_scanset(BDD, int**, int*);
    BDD      bdd_makeset(int *, int);
    bddPair* bdd_copypair(bddPair*);
    bddPair* bdd_mergepairs(bddPair*, bddPair*);
    bddPair* bdd_newpair(void);
    int      bdd_setpair(bddPair*, int, int);
    int      bdd_setpairs(bddPair*, int*, int*, int);
    int      bdd_setbddpair(bddPair*, int, BDD);
    int      bdd_setbddpairs(bddPair*, int*, BDD*, int);
    void     bdd_resetpair(bddPair *);
    void     bdd_freepair(bddPair*);

    int    bdd_error(int) const;
    int    bdd_makenode(unsigned int, int, int);
    int    bdd_noderesize(int);
    void   bdd_checkreorder(void);
    void   bdd_mark(int);
    void   bdd_mark_upto(int, int);
    void   bdd_markcount(int, int*);
    void   bdd_unmark(int);
    void   bdd_unmark_upto(int, int);
    void   bdd_register_pair(bddPair*);
    int   *fdddec2bin(int, int);

    int    bdd_operator_init(int);
    void   bdd_operator_done(void);
    void   bdd_operator_varresize(void);
    void   bdd_operator_reset(void);

    void   bdd_pairs_init(void);
    void   bdd_pairs_done(void);
    int    bdd_pairs_resize(int,int);
    void   bdd_pairs_vardown(int);

    void   bdd_fdd_init(void);
    void   bdd_fdd_done(void);

    void   bdd_reorder_init(void);
    void   bdd_reorder_done(void);
    int    bdd_reorder_ready(void) __purefn;
    void   bdd_reorder_auto(void);
    int    bdd_reorder_vardown(int);
    int    bdd_reorder_varup(int);

    void   bdd_cpp_init(void);


    /*********************************************************************** bddop */
    int      bdd_setcacheratio(int);
    BDD      bdd_buildcube(int, int, BDD *);
    BDD      bdd_ibuildcube(int, int, int *);
    BDD      bdd_not(BDD);
    BDD      bdd_apply(BDD, BDD, int);
    BDD      bdd_and(BDD, BDD);
    BDD      bdd_or(BDD, BDD);
    BDD      bdd_xor(BDD, BDD);
    BDD      bdd_imp(BDD, BDD);
    BDD      bdd_biimp(BDD, BDD);
    BDD      bdd_setxor(BDD, BDD);
    int      bdd_implies(BDD, BDD);
    BDD      bdd_ite(BDD, BDD, BDD);
    BDD      bdd_restrict(BDD, BDD);
    BDD      bdd_constrain(BDD, BDD);
    BDD      bdd_replace(BDD, bddPair*);
    BDD      bdd_compose(BDD, BDD, BDD);
    BDD      bdd_veccompose(BDD, bddPair*);
    BDD      bdd_simplify(BDD, BDD);
    BDD      bdd_exist(BDD, BDD);
    BDD      bdd_existcomp(BDD, BDD);
    BDD      bdd_forall(BDD, BDD);
    BDD      bdd_forallcomp(BDD, BDD);
    BDD      bdd_unique(BDD, BDD);
    BDD      bdd_uniquecomp(BDD, BDD);
    BDD      bdd_appex(BDD, BDD, int, BDD);
    BDD      bdd_appexcomp(BDD, BDD, int, BDD);
    BDD      bdd_appall(BDD, BDD, int, BDD);
    BDD      bdd_appallcomp(BDD, BDD, int, BDD);
    BDD      bdd_appuni(BDD, BDD, int, BDD);
    BDD      bdd_appunicomp(BDD, BDD, int, BDD);
    BDD      bdd_support(BDD);
    BDD      bdd_satone(BDD);
    BDD      bdd_satoneset(BDD, BDD, BDD);
    BDD      bdd_fullsatone(BDD);
    BDD	bdd_satprefix(BDD *);
    void     bdd_allsat(BDD r, bddallsathandler handler);
    double   bdd_satcount(BDD);
    double   bdd_satcountset(BDD, BDD);
    double   bdd_satcountln(BDD);
    double   bdd_satcountlnset(BDD, BDD);
    int      bdd_nodecount(BDD);
    int      bdd_anodecount(BDD *, int);
    int*     bdd_varprofile(BDD);
    double   bdd_pathcount(BDD);

    /*********************************************************************** bddio */
    void     bdd_printall(void);
    void     bdd_fprintall(FILE *);
    void     bdd_fprinttable(FILE *, BDD);
    void     bdd_printtable(BDD);
    void     bdd_fprintset(FILE *, BDD);
    void     bdd_printset(BDD);
    int      bdd_fnprintdot(char *, BDD);
    void     bdd_fprintdot(FILE *, BDD);
    void     bdd_printdot(BDD);
    int      bdd_fnsave(char *, BDD);
    int      bdd_save(FILE *, BDD);
    int      bdd_fnload(char *, BDD *);
    int      bdd_load(FILE *ifile, BDD *);


    /*********************************************************************** fdd */
    int  fdd_extdomain(int*, int);
    int  fdd_overlapdomain(int, int);
    void fdd_clearall(void);
    int  fdd_domainnum(void);
    int  fdd_domainsize(int);
    int  fdd_varnum(int);
    int* fdd_vars(int);
    BDD  fdd_ithvar(int, int);
    int  fdd_scanvar(BDD, int);
    int* fdd_scanallvar(BDD);
    BDD  fdd_ithset(int);
    BDD  fdd_domain(int);
    BDD  fdd_equals(int, int);
    bddfilehandler fdd_file_hook(bddfilehandler);
    bddstrmhandler fdd_strm_hook(bddstrmhandler);
    void fdd_printset(BDD);
    void fdd_fprintset(FILE*, BDD);
    int  fdd_scanset(BDD, int**, int*);
    BDD  fdd_makeset(int*, int);
    int  fdd_intaddvarblock(int, int, int);
    int  fdd_setpair(bddPair*, int, int);
    int  fdd_setpairs(bddPair*, int*, int*, int);

    /************************************************************************ prime */
    unsigned int bdd_prime_gte(unsigned int src);
    unsigned int bdd_prime_lte(unsigned int src);

    /*********************************************************************** reorder */
    int      bdd_swapvar(int v1, int v2);
    void     bdd_default_reohandler(int);
    void     bdd_reorder(int);
    int      bdd_reorder_gain(void) __purefn;
    bddsizehandler bdd_reorder_probe(bddsizehandler);
    void     bdd_clrvarblocks(void);
    int      bdd_addvarblock(BDD, int);
    int      bdd_intaddvarblock(int, int, int);
    void     bdd_varblockall(void);
    bddfilehandler bdd_blockfile_hook(bddfilehandler);
    int      bdd_autoreorder(int);
    int      bdd_autoreorder_times(int, int);
    int      bdd_var2level(int);
    int      bdd_level2var(int);
    int      bdd_getreorder_times(void) __purefn;
    int      bdd_getreorder_method(void) __purefn;
    void     bdd_enable_reorder(void);
    void     bdd_disable_reorder(void);
    int      bdd_reorder_verbose(int);
    void     bdd_setvarorder(int *);
    void     bdd_printorder(void);
    void     bdd_fprintorder(FILE *);


    /***************************************************************************** bddtree */

    BddTree *bddtree_new(int);
    void     bddtree_del(BddTree *);
    BddTree *bddtree_addrange(BddTree *, int, int, int, int);
    void     bddtree_print(FILE *, BddTree *, int);

    /**************************************************************************** bvec */

    BVEC bvec_copy(BVEC v);
    BVEC bvec_true(int bitnum);
    BVEC bvec_false(int bitnum);
    BVEC bvec_con(int bitnum, int val);
    BVEC bvec_var(int bitnum, int offset, int step);
    BVEC bvec_varfdd(int var);
    BVEC bvec_varvec(int bitnum, int *var);
    BVEC bvec_coerce(int bitnum, BVEC v);
    int  bvec_isconst(BVEC e) __purefn;
    int  bvec_val(BVEC e) __purefn;
    void bvec_free(BVEC v);
    BVEC bvec_addref(BVEC v);
    BVEC bvec_delref(BVEC v);
    BVEC bvec_map1(BVEC a, BDD (*fun)(BDD));
    BVEC bvec_map2(BVEC a, BVEC b, BDD (*fun)(BDD,BDD));
    BVEC bvec_map3(BVEC a, BVEC b, BVEC c, BDD (*fun)(BDD,BDD,BDD));
    BVEC bvec_add(BVEC left, BVEC right);
    BVEC bvec_sub(BVEC left, BVEC right);
    BVEC bvec_mulfixed(BVEC e, int c);
    BVEC bvec_mul(BVEC left, BVEC right);
    int  bvec_divfixed(BVEC e, int c, BVEC *res, BVEC *rem);
    int  bvec_div(BVEC left, BVEC right, BVEC *res, BVEC *rem);
    BVEC bvec_ite(BDD a, BVEC b, BVEC c);
    BVEC bvec_shlfixed(BVEC e, int pos, BDD c);
    BVEC bvec_shl(BVEC l, BVEC r, BDD c);
    BVEC bvec_shrfixed(BVEC e, int pos, BDD c);
    BVEC bvec_shr(BVEC l, BVEC r, BDD c);
    BDD  bvec_lth(BVEC left, BVEC right);
    BDD  bvec_lte(BVEC left, BVEC right);
    BDD  bvec_gth(BVEC left, BVEC right);
    BDD  bvec_gte(BVEC left, BVEC right);
    BDD  bvec_equ(BVEC left, BVEC right);
    BDD  bvec_neq(BVEC left, BVEC right);

    /****************************************************************************** cache */

    int  BddCache_init(BddCache *, int);
    void BddCache_done(BddCache *);
    int  BddCache_resize(BddCache *, int);
    void BddCache_reset(BddCache *);


  private:
    /* Min. number of nodes (%) that has to be left after a garbage collect
     unless a resize should be done. */
    int minfreenodes=20;


    /*************************************************************** kernel */

    int          bddrunning;            /* Flag - package initialized */
    int          bdderrorcond;          /* Some error condition */
    int          bddnodesize;           /* Number of allocated nodes */
    int          bddmaxnodesize;        /* Maximum allowed number of nodes */
    int          bddmaxnodeincrease;    /* Max. # of nodes used to inc. table */
    BddNode*     bddnodes;          /* All of the bdd nodes */
    int*         bddhash;           /* Unicity hash table */
    int          bddfreepos;        /* First free node */
    int          bddfreenum;        /* Number of free nodes */
    long int     bddproduced;       /* Number of new nodes ever produced */
    int          bddvarnum;         /* Number of defined BDD variables */
    int*         bddrefstack;       /* Internal node reference stack */
    int*         bddrefstacktop;    /* Internal node reference stack top */
    int*         bddvar2level;      /* Variable -> level table */
    int*         bddlevel2var;      /* Level -> variable table */
    jmp_buf      bddexception;      /* Long-jump point for interrupting calc. */
    int          bddresized;        /* Flag indicating a resize of the nodetable */

    bddCacheStat bddcachestats;

    bddstrmhandler strmhandler_bdd;
    bddstrmhandler strmhandler_fdd;

    BDD*     bddvarset;             /* Set of defined BDD variables */
    int      gbcollectnum;          /* Number of garbage collections */
    int      cachesize;             /* Size of the operator caches */
    long int gbcclock;              /* Clock ticks used in GBC */
    int      usednodes_nextreorder; /* When to do reorder next time */
    bddinthandler  err_handler;     /* Error handler */
    bddgbchandler  gbc_handler;     /* Garbage collection handler */
    bdd2inthandler resize_handler;  /* Node-table-resize handler */

    void bdd_gbc_rehash(void);


    /****************************************************************** bddop */
    int applyop;                 /* Current operator for apply */
    int appexop;                 /* Current operator for appex */
    int appexid;                 /* Current cache id for appex */
    int quantid;                 /* Current cache id for quantifications */
    int *quantvarset;            /* Current variable set for quant. */
    int quantvarsetcomp;         /* Should quantvarset be complemented?  */
    int quantvarsetID;           /* Current id used in quantvarset */
    int quantlast;               /* Current last variable to be quant. */
    int replaceid;               /* Current cache id for replace */
    int *replacepair;            /* Current replace pair */
    int replacelast;             /* Current last var. level to replace */
    int composelevel;            /* Current variable used for compose */
    int miscid;                  /* Current cache id for other results */
    int *varprofile;             /* Current variable profile */
    int supportID;               /* Current ID (true value) for support */
    int supportMin;              /* Min. used level in support calc. */
    int supportMax;              /* Max. used level in support calc. */
    int* supportSet;             /* The found support set */
    BddCache applycache;         /* Cache for apply results */
    BddCache itecache;           /* Cache for ITE results */
    BddCache quantcache;         /* Cache for exist/forall results */
    BddCache appexcache;         /* Cache for appex/appall results */
    BddCache replacecache;       /* Cache for replace results */
    BddCache misccache;          /* Cache for other results */
    int cacheratio;
    BDD satPolarity;
    int firstReorder;            /* Used instead of local variable in order
                                  to avoid compiler warning about 'first'
                                  being clobbered by setjmp */
    char*            allsatProfile; /* Variable profile for bdd_allsat() */
    bddallsathandler allsatHandler; /* Callback handler for bdd_allsat() */


    BDD    not_rec(BDD);
    BDD    apply_rec(BDD, BDD);
    BDD    ite_rec(BDD, BDD, BDD);
    int    simplify_rec(BDD, BDD);
    int    quant_rec(int);
    int    appquant_rec(int, int);
    int    restrict_rec(int);
    BDD    constrain_rec(BDD, BDD);
    BDD    replace_rec(BDD);
    BDD    bdd_correctify(int, BDD, BDD);
    BDD    compose_rec(BDD, BDD);
    BDD    veccompose_rec(BDD);
    void   support_rec(int, int*);
    BDD    satone_rec(BDD);
    BDD    satoneset_rec(BDD, BDD);
    int    fullsatone_rec(int);
    BDD    satprefix_rec(BDD*);
    void   allsat_rec(BDD r);
    double satcount_rec(int);
    double satcountln_rec(int);
    void   varprofile_rec(int);
    double bdd_pathcount_rec(BDD);
    int    varset2vartable(BDD, int);
    int    varset2svartable(BDD);
    void bdd_operator_noderesize(void);
    void checkresize(void);
    BDD quantify(BDD r, BDD var, int op, int comp, int id);
    BDD appquantify(BDD l, BDD r, int opr, BDD var,
                    int qop, int comp, int qid);


    /***************************************************************************** fdd */
    void fdd_printset_rec(FILE *, int, int *);
    void Domain_allocate(Domain*, int);
    void Domain_done(Domain*);
    int    firstbddvar;
    int    fdvaralloc;         /* Number of allocated domains */
    int    fdvarnum;           /* Number of defined domains */
    Domain *domain;            /* Table of domain sizes */
    bddfilehandler filehandler;
    void fdd_printset_rec(std::ostream &o, int r, int *set);



    /************************************************************************ bddio */

    void bdd_printset_rec(FILE *, int, int *);
    void bdd_fprintdot_rec(FILE*, BDD);
    int  bdd_save_rec(FILE*, int);
    int  bdd_loaddata(FILE *);
    int  loadhash_get(int) const;
    void loadhash_add(int, int);

    typedef struct s_LoadHash
    {
        int key;
        int data;
        int first;
        int next;
    } LoadHash;

    LoadHash *lh_table;
    int       lh_freepos;
    int       lh_nodenum;
    int      *loadvar2level;

    /*********************************************************************** pairs */
    int      pairsid;            /* Pair identifier */
    bddPair* pairs;              /* List of all replacement pairs in use */
    int update_pairsid(void);
    bddPair *bdd_pairalloc(void);

    /************************************************************************ prime */

    long u64_mul(unsigned int x, unsigned int y);
    void u64_shl(long* a, unsigned int *carryOut);
    unsigned int u64_mod(long dividend, unsigned int divisor);
    unsigned int numberOfBits(unsigned int src);
    int isWitness(unsigned int witness, unsigned int src);
    int isMillerRabinPrime(unsigned int src);
    int hasEasyFactors(unsigned int src);
    int isPrime(unsigned int src);

    /************************************************************************* reorder */
    BddTree *reorder_win2(BddTree *t);
    BddTree *reorder_win2ite(BddTree *t);
    BddTree *reorder_swapwin3(BddTree *, BddTree **first);
    BddTree *reorder_win3(BddTree *t);
    BddTree *reorder_win3ite(BddTree *t);
    void reorder_sift_bestpos(BddTree *blk, int middlePos);
    BddTree *reorder_sift_seq(BddTree *t, BddTree **seq, int num);
    static int siftTestCmp(const void *aa, const void *bb);
    BddTree *reorder_sift(BddTree *t);
    BddTree *reorder_siftite(BddTree *t);
    BddTree *reorder_random(BddTree *t);
    void blockdown(BddTree *left);
    void addref_rec(int r, char *dep);
    void addDependencies(char *dep);
    int mark_roots(void);
    void reorder_gbc(void);
    void reorder_setLevellookup(void);
    void reorder_rehashAll(void);
    int reorder_makenode(int var, int low, int high);
    int reorder_downSimple(int var0);
    void reorder_swap(int toBeProcessed, int var0);
    void reorder_localGbc(int var0);
    void reorder_swapResize(int toBeProcessed, int var0);
    void reorder_localGbcResize(int toBeProcessed, int var0);
    int reorder_varup(int var);
    void sanitycheck(void);
    int reorder_vardown(int var);
    int reorder_init(void);
    void reorder_done(void);
    int varseqCmp(const void *aa, const void *bb);
    BddTree *reorder_block(BddTree *t, int method);
    void print_order_rec(FILE *o, BddTree *t, int level);

    /* Current auto reord. method and number of automatic reorderings left */
    int bddreordermethod;
    int bddreordertimes;

    /* Flag for disabling reordering temporarily */
    int reorderdisabled;

    /* Store for the variable relationships */
    BddTree *vartree;
    int blockid;

    /* Store for the ref.cou. of the external roots */
    int *extroots;
    int extrootsize;

    /* Level data */
    typedef struct _levelData
    {
        int start;    /* Start of this sub-table (entry in "bddnodes") */
        int size;     /* Size of this sub-table */
        int maxsize;  /* Max. allowed size of sub-table */
        int nodenum;  /* Number of nodes in this level */
    } levelData;

    levelData *levels; /* Indexed by variable! */

    /* Interaction matrix */
    imatrix iactmtx;

    /* Reordering information for the user */
    int verbose;
    bddinthandler reorder_handler_ptr;
    bddfilehandler reorder_filehandler;
    bddsizehandler reorder_nodenum_ptr;
    int reorder_nodenum ();
    void reorder_handler (int);

    /* Number of live nodes before and after a reordering session */
    int usednum_before;
    int usednum_after;

    /* Flag telling us when a node table resize is done */
    int resizedInMakenode;
    /****************************************************************************** bddtree */
    BddTree *bddtree_addrange_rec(BddTree *, BddTree *, int, int, int, int);
    void update_seq(BddTree *t);
    /****************************************************************************** bvec */

    bvec bvec_build(int bitnum, int isTrue);
    void bvec_div_rec(bvec divisor, bvec *remainder, bvec *result, int step);
};

#include "bddio.hxx"
#include "bddop.hxx"
#include "bddtree.hxx"
#include "bvec.hxx"
#include "cache.hxx"
#include "fdd.hxx"
#include "kernel.hxx"
#include "pairs.hxx"
#include "prime.hxx"
#include "reorder.hxx"

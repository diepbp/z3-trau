/*++
Copyright (c) 2006 Microsoft Corporation

Module Name:

    smt_enode.h

Abstract:

    <abstract>

Author:

    Leonardo de Moura (leonardo) 2008-02-18.

Revision History:

--*/
#ifndef SMT_ENODE_H_
#define SMT_ENODE_H_

#include "ast/ast.h"
#include "smt/smt_types.h"
#include "smt/smt_eq_justification.h"
#include "smt/smt_theory_var_list.h"
#include "util/approx_set.h"

namespace smt {
    /**
       \brief Justification for the transitivity rule.
    */
    struct trans_justification {
        enode *           m_target;
        eq_justification  m_justification;
        trans_justification():
            m_target(nullptr),
            m_justification(null_eq_justification) {
        }
    };

    /** \ brief Use sparse maps in SMT solver.

    Define this to use hash maps rather than vectors over ast
    nodes. This is useful in the case there are many solvers, each
    referencing few nodes from a large ast manager. There is some
    unknown performance penalty for this. */

    // #define SPARSE_MAP

#ifndef SPARSE_MAP
    typedef ptr_vector<enode> app2enode_t;    // app -> enode
#else
    class app2enode_t : public u_map<enode *> {
    public:
        void setx(unsigned x, enode *val, enode *def){
            if (val == 0)
                erase(x);
            else
                insert(x,val);
      }
    };
#endif

    class tmp_enode;

    /**
       \brief Additional data-structure for implementing congruence closure,
       equality propagation, and the theory central bus of equalities.
    */
    class enode {
        app  *              m_owner;    //!< The application that 'owns' this enode.
        enode *             m_root;     //!< Representative of the equivalence class
        enode *             m_next;     //!< Next element in the equivalence class.
        enode *             m_cg;       
        unsigned            m_class_size;    //!< Size of the equivalence class if the enode is the root.
        unsigned            m_generation; //!< Tracks how many quantifier instantiation rounds were needed to generate this enode.

        unsigned            m_func_decl_id; //!< Id generated by the congruence table for fast indexing.

        unsigned            m_mark:1;        //!< Multi-purpose auxiliary mark. 
        unsigned            m_mark2:1;       //!< Multi-purpose auxiliary mark. 
        unsigned            m_interpreted:1; //!< True if the node is an interpreted constant.
        unsigned            m_suppress_args:1;  //!< True if the arguments of m_owner should not be accessed by this enode.
        unsigned            m_eq:1;             //!< True if it is an equality
        unsigned            m_commutative:1;    //!< True if commutative app
        unsigned            m_bool:1;           //!< True if it is a boolean enode
        unsigned            m_merge_tf:1;       //!< True if the enode should be merged with true/false when the associated boolean variable is assigned.
        unsigned            m_cgc_enabled:1;    //!< True if congruence closure is enabled for this enode.
        unsigned            m_iscope_lvl;       //!< When the enode was internalized
        /*
          The following property is valid for m_parents
          
          If this = m_root, then for every term f(a) such that a->get_root() == m_root,
          there is a f(b) in m_parents such that b->get_root() == m_root, and f(a) and f(b) are
          congruent.
          Remark: f(a) and f(b) may have other arguments.

          Exception: If f(a) and f(b) are terms of the form (= a c) and (= b d), then
          m_parents will not contains (= b d) if b->get_root() == d->get_root().

          Remark regarding relevancy propagation: relevancy is propagated to all
          elements of an equivalence class. So, if there is a f(a) that is relevant,
          then the congruent f(b) in m_parents will also be relevant. 
        */
        enode_vector        m_parents;          //!< Parent enodes of the equivalence class.
        theory_var_list     m_th_var_list;      //!< List of theories that 'care' about this enode.
        trans_justification m_trans;            //!< A justification for the enode being equal to its root.
        bool                m_proof_is_logged;  //!< Indicates that the proof for the enode being equal to its root is in the log.
        signed char         m_lbl_hash;         //!< It is different from -1, if enode is used in a pattern
        approx_set          m_lbls;
        approx_set          m_plbls;
        enode *             m_args[0];          //!< Cached args
        
        friend class context;
        friend class euf_manager;
        friend class conflict_resolution;
        friend class quantifier_manager;
        

        theory_var_list * get_th_var_list() { 
            return m_th_var_list.get_th_var() == null_theory_var ? nullptr : &m_th_var_list;
        }

        friend class set_merge_tf_trail;
        /**
           \brief Return true if the enode should be merged with the true (false) enodes when
           the associated boolean variable is assigned to true (false).
        */
        bool merge_tf() const {
            return m_merge_tf;
        }

        friend class add_th_var_trail;
        friend class replace_th_var_trail;

        void add_th_var(theory_var v, theory_id id, region & r);

        void replace_th_var(theory_var v, theory_id id);

        void del_th_var(theory_id id);

        friend class tmp_enode;

        static enode * init(ast_manager & m, void * mem, app2enode_t const & app2enode, app * owner, 
                            unsigned generation, bool suppress_args, bool merge_tf, unsigned iscope_lvl,
                            bool cgc_enabled, bool update_children_parent);
    public:

        static unsigned get_enode_size(unsigned num_args) {
            return sizeof(enode) + num_args * sizeof(enode*);
        }
        
        static enode * mk(ast_manager & m, region & r, app2enode_t const & app2enode, app * owner, 
                          unsigned generation, bool suppress_args, bool merge_tf, unsigned iscope_lvl,
                          bool cgc_enabled, bool update_children_parent);

        static enode * mk_dummy(ast_manager & m, app2enode_t const & app2enode, app * owner);
        
        static void del_dummy(enode * n) { dealloc_svect(reinterpret_cast<char*>(n)); }

        unsigned get_func_decl_id() const {
            return m_func_decl_id;
        }

        void set_func_decl_id(unsigned id) {
            m_func_decl_id = id;
        }

        void mark_as_interpreted() {
            SASSERT(!m_interpreted);
            SASSERT(m_owner->get_num_args() == 0);
            SASSERT(m_class_size == 1);
            m_interpreted = true;
        }


        void del_eh(ast_manager & m, bool update_children_parent = true);
        
        app * get_owner() const { 
            return m_owner; 
        }

        unsigned get_owner_id() const {
            return m_owner->get_id();
        }

        func_decl * get_decl() const {
            return m_owner->get_decl();
        }

        unsigned get_decl_id() const {
            return m_owner->get_decl()->get_decl_id();
        }

        unsigned hash() const {
            return m_owner->hash();
        }


        enode * get_root() const { 
            return m_root; 
        }

        enode * get_next() const { 
            return m_next; 
        }

        unsigned get_num_args() const { 
            return m_suppress_args ? 0 : m_owner->get_num_args(); 
        }

        enode * get_arg(unsigned idx) const {
            SASSERT(idx < get_num_args());
            return m_args[idx];
        }

        enode * const * get_args() const {
            return m_args;
        }

        class const_args {
            enode const& n;
        public:
            const_args(enode const& n):n(n) {}
            const_args(enode const* n):n(*n) {}
            enode_vector::const_iterator begin() const { return n.m_args; }
            enode_vector::const_iterator end() const { return n.m_args + n.get_num_args(); }
        };

        class args {
            enode & n;
        public:
            args(enode & n):n(n) {}
            args(enode * n):n(*n) {}
            enode_vector::iterator begin() const { return n.m_args; }
            enode_vector::iterator end() const { return n.m_args + n.get_num_args(); }
        };

        const_args get_const_args() const { return const_args(this); }

        // args get_args() { return args(this); }

        // unsigned get_id() const { 
        //    return m_id; 
        // }

        unsigned get_class_size() const { 
            return m_class_size; 
        }

        bool is_bool() const {
            return m_bool;
        }

        bool is_eq() const { 
            return m_eq; 
        }

        bool is_true_eq() const {
            return m_eq && get_arg(0)->get_root() == get_arg(1)->get_root();
        }

        bool is_marked() const { 
            return m_mark; 
        }

        void set_mark() { 
            SASSERT(!m_mark); m_mark = true; 
        }

        void unset_mark() { 
            SASSERT(m_mark); m_mark = false; 
        }

        bool is_marked2() const { 
            return m_mark2; 
        }

        void set_mark2() { 
            SASSERT(!m_mark2); m_mark2 = true; 
        }

        void unset_mark2() { 
            SASSERT(m_mark2); m_mark2 = false; 
        }

        bool is_interpreted() const { 
            return m_interpreted; 
        }

        /**
           \brief Return true if node is not a constant and it is the root of its congruence class.
           
           \remark if get_num_args() == 0, then is_cgr() = false.
        */
        bool is_cgr() const {
            return m_cg == this;
        }

        enode * get_cg() const { 
            return m_cg;
        }

        bool is_cgc_enabled() const {
            return m_cgc_enabled;
        }

        bool is_commutative() const {
            return m_commutative;
        }

        class const_parents {
            enode const& n;
        public:
            const_parents(enode const& _n):n(_n) {}
            const_parents(enode const* _n):n(*_n) {}
            enode_vector::const_iterator begin() const { return n.begin_parents(); }
            enode_vector::const_iterator end() const { return n.end_parents(); }
        };

        class parents {
            enode& n;
        public:
            parents(enode & _n):n(_n) {}
            parents(enode * _n):n(*_n) {}
            enode_vector::iterator begin() const { return n.begin_parents(); }
            enode_vector::iterator end() const { return n.end_parents(); }
        };

        parents get_parents() { return parents(this); }

        const_parents get_const_parents() const { return const_parents(this); }

        unsigned get_num_parents() const {
            return m_parents.size();
        }

        enode_vector::iterator begin_parents() { 
            return m_parents.begin(); 
        }

        enode_vector::iterator end_parents() { 
            return m_parents.end(); 
        }

        enode_vector::const_iterator begin_parents() const { 
            return m_parents.begin(); 
        }
        
        enode_vector::const_iterator end_parents() const { 
            return m_parents.end(); 
        }
        
        theory_var_list const * get_th_var_list() const { 
            return m_th_var_list.get_th_var() == null_theory_var ? nullptr : &m_th_var_list;
        }

        bool has_th_vars() const {
            return m_th_var_list.get_th_var() != null_theory_var;
        }

        unsigned get_num_th_vars() const;

        theory_var get_th_var(theory_id th_id) const;

        trans_justification get_trans_justification() {
            return m_trans;
        }

        unsigned get_generation() const {
            return m_generation;
        }

        void set_generation(context & ctx, unsigned generation);
        
        /**
           \brief Return the enode n that is in the eqc of *this, and has the minimal generation.
           That is, there is no other enode with smaller generation.
        */
        enode * get_eq_enode_with_min_gen();

        unsigned get_iscope_lvl() const {
            return m_iscope_lvl;
        }

        void set_lbl_hash(context & ctx);
        
        bool has_lbl_hash() const {
            return m_lbl_hash >= 0;
        }
        
        unsigned char get_lbl_hash() const {
            SASSERT(m_lbl_hash >= 0 && static_cast<unsigned>(m_lbl_hash) < approx_set_traits<unsigned long long>::capacity);
            return static_cast<unsigned char>(m_lbl_hash);
        }
        
        approx_set & get_lbls() {
            return m_lbls;
        }
        
        approx_set & get_plbls() {
            return m_plbls;
        }
        
        const approx_set & get_lbls() const {
            return m_lbls;
        }
        
        const approx_set & get_plbls() const {
            return m_plbls;
        }

        void display_lbls(std::ostream & out) const;
        
#ifdef Z3DEBUG
        bool check_invariant() const;
        bool trans_reaches(enode * n) const;
        bool check_parent_invariant() const;
        bool contains_parent_congruent_to(enode * p) const;
#endif
    };

    inline bool same_eqc(enode const * n1 , enode const * n2) { return n1->get_root() == n2->get_root(); }

    /**
       \brief Return true, if n1 and n2 are congruent.
       Set comm to true, if the nodes are congruent modulo commutativity.
    */
    bool congruent(enode * n1, enode * n2, bool & comm);

    inline bool congruent(enode * n1, enode * n2) {
        bool aux;
        return congruent(n1, n2, aux);
    }

    unsigned get_max_generation(unsigned num_enodes, enode * const * enodes);
    
    void unmark_enodes(unsigned num_enodes, enode * const * enodes);

    void unmark_enodes2(unsigned num_enodes, enode * const * enodes);
    
    class tmp_enode {
        tmp_app  m_app;
        unsigned m_capacity;
        char *   m_enode_data;
        enode * get_enode() { return reinterpret_cast<enode*>(m_enode_data); }
        void set_capacity(unsigned new_capacity);
    public:
        tmp_enode();
        ~tmp_enode();
        enode * set(func_decl * f, unsigned num_args, enode * const * args);
        void reset();
    };

};

#endif /* SMT_ENODE_H_ */


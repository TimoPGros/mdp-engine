/*
 *  Copyright (c) 2011-2016 Universidad Simon Bolivar
 * 
 *  Permission is hereby granted to distribute this software for
 *  non-commercial research purposes, provided that this copyright
 *  notice is included with any such distribution.
 *  
 *  THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 *  EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE
 *  SOFTWARE IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU
 *  ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.
 *  
 *  Blai Bonet, bonet@ldc.usb.ve
 *
 */

#ifndef IW_BEL_H
#define IW_BEL_H

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <limits>
#include <vector>
#include <math.h>

#include "pomdp.h"

#define DEBUG

namespace Online {

namespace Policy {

namespace IWBel {

////////////////////////////////////////////////
//
// Nodes
//

template<typename T> struct node_t {
    const T belief_;
    const POMDP::feature_t<T> *feature_;
    const node_t<T> *parent_;
    Problem::action_t a_;
    float g_;

    node_t(const T &belief, const POMDP::feature_t<T> *feature, const node_t<T> *parent = 0, Problem::action_t a = Problem::noop, float cost = 0)
      : belief_(belief), feature_(feature), parent_(parent), a_(a), g_(0) {
        if( parent_ != 0 ) g_ = parent_->g_ + cost;
    }
    ~node_t() { }

    void print(std::ostream &os) const {
        os << "[bel=" << belief_ << ",a=" << a_ << ",g=" << g_ << ",pa=" << parent_ << "]";
    }
};

template<typename T> inline std::ostream& operator<<(std::ostream &os, const node_t<T> &node) {
    node.print(os);
    return os;
}

template<typename T> inline node_t<T>* get_root_node(const T &belief, const POMDP::feature_t<T> *feature) {
    return new node_t<T>(belief, feature);
}

template<typename T> inline node_t<T>* get_node(const T &belief, const POMDP::feature_t<T> *feature, const node_t<T> *parent, Problem::action_t a, float cost) {
    return new node_t<T>(belief, feature, parent, a, cost);
}

////////////////////////////////////////////////
//
// Hash Table
//

template<typename T> struct node_map_function_t {
    size_t operator()(const T *bel) const {
        return bel->hash();
    }
    bool operator()(const T *b1, const T *b2) const {
        return *b1 == *b2;
    }
};

#if 0
struct data_t {
    std::vector<float> values_;
    std::vector<int> counts_;
    data_t(const std::vector<float> &values, const std::vector<int> &counts)
      : values_(values), counts_(counts) { }
    data_t(const data_t &data)
      : values_(data.values_), counts_(data.counts_) { }
    data_t(data_t &&data)
      : values_(std::move(data.values_)), counts_(std::move(data.counts_)) { }
};
#endif

template<typename T> class node_hash_t : public Hash::generic_hash_map_t<const T*, const node_t<T>*, node_map_function_t<T>, node_map_function_t<T> > {
  public:
    typedef typename Hash::generic_hash_map_t<const T*, const node_t<T>*, node_map_function_t<T>, node_map_function_t<T> > base_type;
    typedef typename base_type::const_iterator const_iterator;
    typedef typename base_type::iterator iterator;

  public:
    node_hash_t() { }
    virtual ~node_hash_t() { }
    void print(std::ostream &os) const {
        assert(0); // CHECK
#if 0
        for( const_iterator it = begin(); it != end(); ++it ) {
            os << "(" << it->first.first << "," << it->first.second << ")" << std::endl;
        }
#endif
    }
};

////////////////////////////////////////////////
//
// Policy
//

template<typename T> class iw_bel_t : public policy_t<T> {
  public:
    typedef enum { MOST_LIKELY, SAMPLE } determinization_t;
    typedef enum { REWARD, TARGET } stop_criterion_t;
    typedef enum { TOTAL, KL, KL_SYM, JS, UNKNOWN } divergence_t;
    typedef enum { MAX, ADD } score_aggregation_t;

    static divergence_t divergence_from_name(const std::string &d) {
        if( d == "total" )
            return TOTAL;
        else if( d == "kl" )
            return KL;
        else if( d == "kl-sym" )
            return KL_SYM;
        else if( d == "js" )
            return JS;
        else
            return UNKNOWN;
    }
    static std::string divergence_name(divergence_t d) {
        if( d == TOTAL )
            return "total";
        else if( d == KL )
            return "kl";
        else if( d == KL_SYM )
            return "kl-sym";
        else if( d == JS )
            return "js";
        else
            return "unknown";
    }

  protected:
    const POMDP::pomdp_t<T> &pomdp_;

    unsigned width_;
    determinization_t determinization_;
    stop_criterion_t stop_criterion_;
    divergence_t divergence_;
    unsigned max_expansions_;
    score_aggregation_t score_aggregation_;
    bool random_ties_;

    mutable node_hash_t<T> node_table_;

    struct min_priority_t {
        bool operator()(const node_t<T> *n1, const node_t<T> *n2) const {
            return n1->g_ > n2->g_;
        }
    };
    typedef typename std::priority_queue<const node_t<T>*, std::vector<const node_t<T>*>, min_priority_t> priority_queue_t;

    iw_bel_t(const POMDP::pomdp_t<T> &pomdp,
             unsigned width,
             determinization_t determinization,
             stop_criterion_t stop_criterion,
             divergence_t divergence,
             unsigned max_expansions,
             score_aggregation_t score_aggregation,
             bool random_ties)
      : policy_t<T>(pomdp),
        pomdp_(pomdp),
        width_(width),
        determinization_(determinization),
        stop_criterion_(stop_criterion),
        divergence_(divergence),
        max_expansions_(max_expansions),
        score_aggregation_(score_aggregation),
        random_ties_(random_ties) {
    }

  protected:


  public:
    iw_bel_t(const POMDP::pomdp_t<T> &pomdp)
      : policy_t<T>(pomdp),
        pomdp_(pomdp),
        width_(0),
        determinization_(MOST_LIKELY),
        stop_criterion_(TARGET),
        divergence_(TOTAL),
        max_expansions_(std::numeric_limits<unsigned>::max()),
        score_aggregation_(MAX),
        random_ties_(true) {
    }
    virtual ~iw_bel_t() { }
    virtual policy_t<T>* clone() const {
        return new iw_bel_t(pomdp_);
    }
    virtual std::string name() const {
        return std::string("iw-bel(") +
          std::string("width=") + std::to_string(width_) +
          std::string(",determinization=") + (determinization_ == MOST_LIKELY ? "most_likely" : "sample") +
          std::string(",stop-criterion=") + (stop_criterion_ == TARGET ? "target" : "reward") +
          std::string(",divergence=") + divergence_name(divergence_) +
          std::string(",max-expansions=") + std::to_string(max_expansions_) +
          std::string(",score-aggregation=") + (score_aggregation_ == MAX ? "max" : "add") +
          std::string(",random_ties=") + (random_ties_ ? "true" : "false") +
          std::string(")");
    }

    Problem::action_t operator()(const T &bel) const {
#if 0
        std::cout << std::endl
                  << "**** REQUEST FOR ACTION ****" << std::endl
                  << "bel=" << bel << ", cardinality=" << pomdp_.cardinality(bel) << std::endl
                  << "throwing BFS for goal" << std::endl;
#endif

        if( pomdp_.dead_end(bel) ) return Problem::noop;

        const POMDP::feature_t<T> *feature = pomdp_.get_feature(bel);
        node_t<T> *root = get_root_node(bel, feature);
        //const node_t<T> *goal = pruned_bfs(root);
        const node_t<T> *goal = breadth_first_search(root);

        if( goal == 0 ) {
            // goal node wasn't found, return random action
            std::vector<Problem::action_t> applicable_actions;
            applicable_actions.reserve(pomdp_.number_actions(bel));
            for( Problem::action_t a = 0; a < pomdp_.number_actions(bel); ++a ) {
                if( pomdp_.applicable(bel, a) ) {
                    applicable_actions.push_back(a);
                }
            }
            return applicable_actions.empty() ? Problem::noop : applicable_actions[!random_ties_ ? 0 : Random::random(applicable_actions.size())];
        } else {
            // return first action in path
            const node_t<T> *node = goal;
            const node_t<T> *parent = node->parent_;
            assert(parent != 0);
            while( parent->parent_ != 0 ) {
#if 0
                std::cout << "ACTION=" << node->a_ << std::endl;
#endif
                node = parent;
                parent = node->parent_;
            }
            assert(node->parent_->belief_ == bel);
#if 0
            std::cout << "BEST=" << node->a_ << std::endl;
#endif
            return node->a_;
        }
    }
    virtual void reset_stats() const {
        pomdp_.clear_expansions();
    }
    virtual void print_other_stats(std::ostream &os, int indent) const {
        os << std::setw(indent) << ""
           << "other-stats: name=" << name()
           << " decisions=" << policy_t<T>::decisions_
           << std::endl;
    }
    virtual void set_parameters(const std::multimap<std::string, std::string> &parameters, Dispatcher::dispatcher_t<T> &dispatcher) {
        std::multimap<std::string, std::string>::const_iterator it = parameters.find("width");
        if( it != parameters.end() ) width_ = strtol(it->second.c_str(), 0, 0);
        it = parameters.find("determinization");
        if( it != parameters.end() ) determinization_ = it->second == "sample" ? SAMPLE : MOST_LIKELY;
        it = parameters.find("stop-criterion");
        if( it != parameters.end() ) stop_criterion_ = it->second == "reward" ? REWARD : TARGET;
        it = parameters.find("divergence");
        if( it != parameters.end() ) divergence_ = divergence_from_name(it->second);
        it = parameters.find("max-expansions");
        if( it != parameters.end() ) max_expansions_ = strtol(it->second.c_str(), 0, 0);
        it = parameters.find("score-aggregation");
        if( it != parameters.end() ) score_aggregation_ = it->second == "max" ? MAX : ADD;
        it = parameters.find("random-ties");
        if( it != parameters.end() ) random_ties_ = it->second == "true";
        policy_t<T>::setup_time_ = 0;
#ifdef DEBUG
        std::cout << "debug: iw-bel(): params:"
                  << " width=" << width_
                  << " determinization=" << (determinization_ == MOST_LIKELY ? "most-likely" : "sample")
                  << " stop-criterion=" << (stop_criterion_ == TARGET ? "target" : "reward")
                  << " divergence=" << divergence_name(divergence_)
                  << " max-expansions=" << max_expansions_
                  << " score-aggregation=" << (score_aggregation_ == MAX ? "max" : "add")
                  << " random-ties=" << (random_ties_ ? "true" : "false")
                  << std::endl;
#endif
    }
    virtual typename policy_t<T>::usage_t uses_base_policy() const { return policy_t<T>::usage_t::No; }
    virtual typename policy_t<T>::usage_t uses_heuristic() const { return policy_t<T>::usage_t::No; }
    virtual typename policy_t<T>::usage_t uses_algorithm() const { return policy_t<T>::usage_t::No; }

    void clear_feature_table() const {
        assert(0);
        // CHECK
    }
    bool lookup_feature(const POMDP::feature_t<T> &feature) const {
        assert(0);
        return false; // CHECK
    }
    bool insert_feature(const POMDP::feature_t<T> &feature) const {
        assert(0);
        return false; // CHECK
    }

    void clear_node_table() const {
        for( typename node_hash_t<T>::const_iterator it = node_table_.begin(); it != node_table_.end(); ++it )
            delete it->second;
        node_table_.clear();
    }
    const node_t<T>* lookup_node(const T &belief) const {
        typename node_hash_t<T>::const_iterator it = node_table_.find(&belief);
        //std::cout << "lookup: bel=" << belief << ", found=" << (it == node_table_.end() ? "NO" : "YES") << std::endl;
        return it == node_table_.end() ? 0 : it->second;
    }
    bool insert_node(const node_t<T> *node) const {
        //std::cout << "INSERT: bel=" << node->belief_ << std::endl;
        std::pair<typename node_hash_t<T>::iterator, bool> p = node_table_.insert(std::make_pair(&node->belief_, node));
        return p.second;
    }

    // pruned breadth-first search
    const node_t<T>* pruned_bfs(const node_t<T> *root) const {
        priority_queue_t q;
        std::vector<std::pair<T, float> > outcomes;

        clear_feature_table();
        clear_node_table();

        insert_feature(*root->feature_);
        insert_node(root);
        q.push(root);
        while( !q.empty() ) {
            const node_t<T> *node = q.top();
            q.pop();

            // check whether we need to stop
            if( pomdp_.terminal(node->belief_) ) {
                return node;
            }

            // expand node
            for( Problem::action_t a = 0; a < pomdp_.number_actions(node->belief_); ++a ) {
                if( pomdp_.applicable(node->belief_, a) ) {
                    pomdp_.next(node->belief_, a, outcomes);

                    // select best (unexpanded) successors to continue search
                    int best = -1;
                    for( int i = 0, isz = outcomes.size(); i < isz; ++i ) {
                        if( pomdp_.dead_end(outcomes[i].first) ) continue;
                        if( lookup_node(outcomes[i].first) != 0 ) continue;
                        if( (best == -1) && (outcomes[i].second > outcomes[best].second) ) {
                            best = i;
                        }
                    }

                    // insert in queue if it is novel (CHECK: what happens if it is not novel but there is another successor that is novel?)
                    const POMDP::feature_t<T> *feature = pomdp_.get_feature(outcomes[best].first);
                    if( insert_feature(*feature) ) {
                        float cost = pomdp_.cost(node->belief_, a);
                        node_t<T> *new_node = get_node(outcomes[best].first, feature, node, a, cost);
                        q.push(new_node);
                    }
                }
            }
        }
        return 0;
    }

    // breadth-first search
    const node_t<T>* breadth_first_search(const node_t<T> *root) const {
        std::list<const node_t<T>*> open_list, closed_list;
        std::vector<std::pair<T, float> > outcomes;

        //clear_feature_table();
        clear_node_table();

        //insert_feature(*root->feature_);
        insert_node(root);
        open_list.push_back(root);
        for( unsigned iter = 0; !open_list.empty() && ((stop_criterion_ != TARGET) || (iter < max_expansions_)); ++iter ) {
            const node_t<T> *node = select_node_for_expansion(open_list, closed_list);
            closed_list.push_front(node);

            // check whether we need to stop
            if( (stop_criterion_ == TARGET) && pomdp_.terminal(node->belief_) ) {
                return node;
            }

            // expand node
            for( Problem::action_t a = 0; a < pomdp_.number_actions(node->belief_); ++a ) {
                if( pomdp_.applicable(node->belief_, a) ) {
                    node_t<T> *new_node = 0;
                    const POMDP::feature_t<T> *feature = 0;
                    float cost = pomdp_.cost(node->belief_, a);

                    // determinize the next belief
                    if( determinization_ == SAMPLE ) {
                        std::pair<const T, bool> p = pomdp_.sample_without_hidden(node->belief_, a);
                        feature = pomdp_.get_feature(p.first);
                        new_node = get_node(p.first, feature, node, a, cost);
                    } else {
                        pomdp_.next(node->belief_, a, outcomes);

                        // select best (unexpanded) successors to continue search
                        int best = -1;
                        for( int i = 0, isz = outcomes.size(); i < isz; ++i ) {
                            if( pomdp_.dead_end(outcomes[i].first) ) continue;
                            if( lookup_node(outcomes[i].first) != 0 ) continue;
                            if( (best == -1) || (outcomes[i].second > outcomes[best].second) ) {
                                best = i;
                            }
                        }
                        if( best == -1 ) continue; // no new belief, continue to next action
                        feature = pomdp_.get_feature(outcomes[best].first);
                        new_node = get_node(outcomes[best].first, feature, node, a, cost);
                    }

                    // insert new node into open list
                    insert_node(new_node);
                    open_list.push_back(new_node);
                }
            }
        }
        return 0;
    }

    const node_t<T>* select_node_for_expansion(std::list<const node_t<T>*> &open_list, const std::list<const node_t<T>*> &closed_list) const {
#if 0
        std::cout << "select: ENTRY: open.sz=" << open_list.size() << ", closed.sz=" << closed_list.size() << std::endl;
#endif
        std::vector<typename std::list<const node_t<T>*>::iterator> best_nodes;
        float best_score = std::numeric_limits<float>::min();
        for( typename std::list<const node_t<T>*>::iterator it = open_list.begin(); it != open_list.end(); ++it ) {
            assert(!pomdp_.dead_end((*it)->belief_));
            float node_score = score_aggregation_ == MAX ? std::numeric_limits<float>::min() : 0;
#if 0
            std::cout << "    candidate: a=" << (*it)->a_ << ", bel=" << (*it)->belief_ << ", score=" << std::flush;
#endif
            if( pomdp_.terminal((*it)->belief_) ) {
                node_score = std::numeric_limits<float>::infinity();
            } else {
                for( typename std::list<const node_t<T>*>::const_iterator jt = closed_list.begin(); jt != closed_list.end(); ++jt ) {
                    float s = score(**it, **jt);
                    if( score_aggregation_ == MAX ) {
                        if( s > node_score ) node_score = s;
                    } else {
                        node_score += s;
                    }
                }
            }
            if( best_nodes.empty() || (node_score >= best_score) ) {
                if( node_score > best_score ) best_nodes.clear();
                best_score = node_score;
                best_nodes.push_back(it);
            }
#if 0
            std::cout << node_score << std::endl;
#endif
        }
#if 0
        std::cout << "    best: #=" << best_nodes.size() << ", score=" << best_score << std::endl,
#endif
        assert(!best_nodes.empty());
        typename std::list<const node_t<T>*>::iterator best = best_nodes[!random_ties_ ? 0 : Random::random(best_nodes.size())];
        const node_t<T> *node = *best;
        open_list.erase(best);
#if 0
        std::cout << "select: EXIT: open.sz=" << open_list.size() << ", closed.sz=" << closed_list.size() << ", node-ptr=" << node << ", score=" << best_score << ", node=" << *node << std::endl;
#endif
        return node;
    }

    float score(const node_t<T> &n1, const node_t<T> &n2) const {
        assert(n1.feature_ != 0);
        assert(n2.feature_ != 0);
        const std::vector<std::vector<float> > &m1 = n1.feature_->marginals_;
        const std::vector<std::vector<float> > &m2 = n2.feature_->marginals_;
        assert(m1.size() == m2.size());

        float max_score = 0;
        for( int i = 0, isz = int(m1.size()); i < isz; ++i ) {
            float s = score(m1[i], m2[i]);
            max_score = s > max_score ? s : max_score;
        }
        return max_score;
    }
    float score(const std::vector<float> &d1, const std::vector<float> &d2) const {
        if( divergence_ == TOTAL )
            return score_total(d1, d2);
        else if( divergence_ == KL )
            return score_kl(d1, d2, false);
        else if( divergence_ == KL_SYM )
            return score_kl(d1, d2, true);
        else if( divergence_ == JS )
            return score_js(d1, d2);
        else
            return 0;
    }
    float score_total(const std::vector<float> &p, const std::vector<float> &q) const {
        return Random::total_divergence(p, q);
    }
    float score_kl(const std::vector<float> &p, const std::vector<float> &q, bool symmetric) const {
        return Random::kl_divergence(p, q, symmetric);
    }
    float score_js(const std::vector<float> &p, const std::vector<float> &q) const {
        return Random::js_divergence(p, q);
    }
};

}; // namespace IWBel

}; // namespace Policy

}; // namespace Online

#undef DEBUG

#endif


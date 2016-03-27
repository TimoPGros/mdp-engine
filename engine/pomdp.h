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

#ifndef POMDP_H
#define POMDP_H

#include "hash.h"
#include "problem.h"
#include "random.h"
#include "utils.h"

#include <iostream>
#include <cassert>
#include <limits>
#include <set>
#include <vector>
#include <float.h>

//#define DEBUG

namespace POMDP {

#ifndef __OBSERVATION_TYPE
#define __OBSERVATION_TYPE
typedef int observation_t;
#endif

template<typename T> struct feature_t {
    std::vector<std::vector<float> > marginals_;
    virtual ~feature_t() { }
};

template<typename T> class pomdp_t : public Problem::problem_t<T> {
  protected:
    mutable size_t belief_expansions_;

  public:
    typedef std::vector<int> varset_t;

#if 0
    struct beam_t {
        struct const_iterator {
            virtual bool operator==(const const_iterator &it) const = 0;
            virtual bool operator!=(const const_iterator &it) const = 0;
            virtual const const_iterator& operator++() = 0;
            virtual int value() const = 0;
            virtual float prob() const = 0;
        };

        virtual int cardinality() const = 0;
        virtual const_iterator begin() const = 0;
        virtual const_iterator end() const = 0;
    };
#endif

  public:
    pomdp_t(float discount = 1.0, float dead_end_value = 1e3)
      : Problem::problem_t<T>(discount, dead_end_value),
        belief_expansions_(0) {
    }
    virtual ~pomdp_t() { }

    size_t belief_expansions() const {
        return belief_expansions_;
    }
    void clear_expansions() const {
        belief_expansions_ = 0;
    }


    virtual int num_variables() const = 0;
    virtual int num_beams() const = 0;
    virtual const varset_t& varset(int bid) const = 0;
    //virtual const beam_t& beam(const T &bel, int bid) const = 0;

    virtual int cardinality(const T &bel) const = 0;
    virtual const feature_t<T> *get_feature(const T &bel) const = 0;
    virtual void clean_feature(const feature_t<T> *feature) const = 0;

    virtual void apply_action(T &bel_a, Problem::action_t a) const = 0;
    virtual void apply_obs(T &bel_ao, Problem::action_t a, observation_t obs) const = 0;
    virtual observation_t sample_observation(const T &bel, const T &bel_a, Problem::action_t a) const = 0;

    // sample next state given action using problem's dynamics
    virtual std::pair<T, bool> sample(const T &bel, Problem::action_t a) const {
        T nbel = bel;
        apply_action(nbel, a);
        observation_t obs = sample_observation(bel, nbel, a);
        apply_obs(nbel, a, obs);
#ifdef DEBUG
        std::cout << "pomdp: sampling: obs=" << obs << ", nbel=" << nbel << std::endl;
#endif
        return std::make_pair(nbel, true);
    }

    // sample next state given action uniformly among all possible next states
    virtual std::pair<T, bool> usample(const T &bel, Problem::action_t a) const {
        std::cout << Utils::error() << "this sampling (usample) is not supported for POMDPs" << std::endl;
        exit(0);
    }

    // sample next (unlabeled) state given action; probabilities are re-weighted
    virtual std::pair<T, bool> nsample(const T &bel, Problem::action_t a, const Problem::hash_t<T> &hash) const {
        std::cout << Utils::error() << "this sampling (nsample) is not supported for POMDPs" << std::endl;
        exit(0);
    }

    // print pomdp description
    virtual void print(std::ostream &os) const = 0;
};

}; // namespace POMDP

template<typename T>
inline std::ostream& operator<<(std::ostream &os, const POMDP::pomdp_t<T> &pomdp) {
    pomdp.print(os);
    return os;
}

#undef DEBUG

#endif


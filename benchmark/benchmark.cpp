/* 
 * Copyright (C) 2016  Peter G. Jensen <root@petergjoel.dk>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
// Created by Peter G. Jensen on 12/9/16.

#include <iostream>
#include <ptrie.h>
#include <stdlib.h>
#include <set>
#include <sparsehash/sparse_hash_set>
#include <sparsehash/dense_hash_set>
#include <tbb/concurrent_unordered_set.h>
#include <random>
#include <ptrie_stable.h>
#include "MurmurHash2.h"

using namespace ptrie;

void read_arg(const char* data, auto& dest, const char* error, const char* type)
{
    if(sscanf(data, type, &dest) != 1)
    {
        std::cerr << error << std::endl;
        exit(-1);
    }

}

ptrie::binarywrapper_t rand_data(size_t seed, size_t maxsize, size_t minsize = sizeof(size_t))
{
    assert(minsize >= sizeof(size_t));
    srand(seed);
    // pick size between 0 and maxsize
    size_t size = minsize != maxsize ?
                  minsize + rand() % (maxsize - minsize) :
                  minsize;

    binarywrapper_t data(size * 8);
    // fill in random data
    for (size_t j = 0; j < size; ++j) {
        data.raw()[j] = (uchar) rand();
    }
    // make sure everything is unique
    for (size_t j = 1; j <= sizeof(size_t); ++j) {
        data.raw()[size - j] = ((uchar *) &seed)[j - 1];
    }
    return data;
}

void print_settings(const char* type, size_t elements, size_t seed, size_t bytes, double deletes)
{
    std::cout << "Using " << type << ", inserting " << elements << " items of "
              << bytes << " bytes produced via seed " << seed << ". Of those "
              << (deletes*100.0) << "% will be deleted at random" << std::endl;
}

struct wrapper_t
{
    binarywrapper_t data;
    uint64_t _hash;

    bool operator <(const wrapper_t& other) const
    {
        if(_hash == other._hash)
        {
            if(data.size() == other.data.size()) {
                return memcmp(data.const_raw(), other.data.const_raw(), data.size()) > 0;
            }
            else
            {
                return data.size() < other.data.size();
            }
        }
        return _hash < other._hash;
    }

};

void set_insert(auto& set, size_t elements, size_t seed, size_t bytes, double deletes)
{
    std::default_random_engine generator(seed);
    std::uniform_real_distribution<double> dist;
    std::uniform_int_distribution<int>  rem(0, elements);

    wrapper_t w;
    for(size_t i = 0; i < elements; ++i)
    {
        w.data = rand_data(seed + i, bytes, bytes);
        w._hash = MurmurHash64A(w.data.raw(), w.data.size(), seed);
        set.insert(w);
/*        if(dist(generator) < deletes)
        {
            size_t el = rem(generator) % elements;
            w.data = rand_data(seed + el, bytes, bytes);
            w._hash = MurmurHash64A(w.data.raw(), w.data.size(), seed);
            auto it = set.find(w);
            if(it != set.end())
                set.erase(it);
            w.data.release();
        }*/
    }
    w.data = binarywrapper_t();
}

void set_insert_ptrie(auto& set, size_t elements, size_t seed, size_t bytes, double deletes)
{
    std::default_random_engine generator(seed);
    std::uniform_real_distribution<double> dist;
    std::uniform_int_distribution<int>  rem(0, elements);

    for(size_t i = 0; i < elements; ++i)
    {
        auto data = rand_data(seed + i, bytes, bytes);
        set.insert(data);
        data.release();
        /*if(dist(generator) < deletes)
        {
            size_t el = rem(generator) % elements;
            auto torem = rand_data(seed + el, bytes, bytes);
            set.erase(torem);
            torem.release();
        }*/
    }
}


struct hasher_o
{
    uint64_t operator()(const wrapper_t& w) const
    {
        return w._hash;
    }
};

struct equal_o
{
    bool operator()(const wrapper_t& w1, const wrapper_t& w2) const
    {
        if(w1._hash == w2._hash)
        {
            return w1.data == w2.data;
        }
        else
        {
            return false;
        }
    }
};

int main(int argc, const char** argv)
{
    if(argc < 3 || argc > 6)
    {
        std::cout << "usage : <ptrie/std/sparse/dense> <number elements> <?seed> <?number of bytes> <?delete ratio>" << std::endl;
        exit(-1);
    }

    const char* type = argv[1];
    size_t elements = 1024;
    size_t seed = 0;
    size_t bytes = 16;
    double deletes = 0.0;

    read_arg(argv[2], elements, "Error in <number of elements>", "%zu");
    if(argc > 3) read_arg(argv[3], seed, "Error in <seed>", "%zu");
    if(argc > 4) read_arg(argv[4], bytes, "Error in <bytes>", "%zu");
    if(argc > 5) read_arg(argv[5], deletes, "Error in <deletes>", "%lf");

    if(strcmp(type, "ptrie") == 0)
    {
        print_settings(type, elements, seed, bytes, deletes);
        set<> set;
        set_insert_ptrie(set, elements, seed, bytes, deletes);
    }
    else if (strcmp(type, "std") == 0) {
        print_settings(type, elements, seed, bytes, deletes);
        std::set<wrapper_t> set;
        set_insert(set, elements, seed, bytes, deletes);
    }
    else if(strcmp(type, "tbb") == 0)
    {
        print_settings(type, elements, seed, bytes, deletes);
        tbb::concurrent_unordered_set<wrapper_t, hasher_o, equal_o> set;
        set_insert(set, elements, seed, bytes, deletes);

    }
    else if(strcmp(type, "sparse") == 0)
    {
        print_settings(type, elements, seed, bytes, deletes);
        google::sparse_hash_set<wrapper_t, hasher_o, equal_o> set(elements/10);
        set_insert(set, elements, seed, bytes, deletes);
    }
    else if(strcmp(type, "dense") == 0)
    {
        google::dense_hash_set<wrapper_t, hasher_o, equal_o> set(elements/10);
        wrapper_t empty;
        empty._hash = 0;
        wrapper_t del;
        del._hash = std::numeric_limits<uint64_t>::max();
        set.set_empty_key(empty);
        if(deletes > 0.0) set.set_deleted_key(del);
        set_insert(set, elements, seed, bytes, deletes);
        print_settings(type, elements, seed, bytes, deletes);
    }
    else
    {
        std::cerr << "ERROR IN TYPE, ONLY VALUES ALLOWED : ptrie, std, sparse, dense" << std::endl;
        exit(-1);
    }

    return 0;
}
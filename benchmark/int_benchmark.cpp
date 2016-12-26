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
// Created by Peter G. Jensen on 24/12/16.

#include <iostream>
#include <ptrie.h>
#include <stdlib.h>
#include <sparsehash/sparse_hash_set>
#include <sparsehash/dense_hash_set>
#include <tbb/concurrent_unordered_set.h>
#include <random>
#include <ptrie.h>
#include <chrono>
#include <unordered_set>
#include <vector>
#include <set>
#include "MurmurHash2.h"
#include "utils.h"

size_t reorder(size_t el, std::vector<size_t>& order, size_t seed)
{
    el = el ^ seed;
    ptrie::binarywrapper_t s((ptrie::uchar*)&el, sizeof(size_t)*8);
    ptrie::binarywrapper_t t(sizeof(size_t)*8);

    size_t& target = *(size_t*)t.raw();
    bool flip[8];
    for(size_t i = 0; i < 8; ++i) flip[i] = s.at(i);
    for(size_t i = 0; i < sizeof(size_t)*8; ++i)
    {
        if(i <= 7)
        {
            t.set(order[i], s.at(i));
        }
        else
        {
            t.set(order[i], flip[i % 8] xor s.at(i));
        }
        for(size_t j = 0; j < 8; ++j) flip[j] = flip[j] xor t.at(order[i]);
    }
    std::cout << s << std::endl;
    std::cout << t << std::endl;
    std::cout << el << " -> " << target << std::endl;
    return target;
}


void set_insert(auto& set, size_t elements, size_t seed, double deletes, double read_rate, std::vector<size_t>& order)
{
    std::default_random_engine generator(seed);
    std::uniform_real_distribution<double> dist;

    std::default_random_engine read_generator(seed);
    std::normal_distribution<double> read_dist(read_rate, read_rate / 2.0);
    std::uniform_int_distribution<size_t> read_el(1, elements);


    auto start = std::chrono::system_clock::now();
    for(size_t i = 1; i <= elements; ++i)
    {
        size_t val = reorder(i, order, seed);
        set.insert(val);

        if(read_rate > 0.0)
        {
            int reads = std::round(read_dist(read_generator));
            for(int r = 0; r < reads; ++r)
            {
                size_t el = reorder(read_el(read_generator), order, seed);
                set.count(el);
            }
        }

        if(dist(generator) < deletes)
        {
            std::uniform_int_distribution<size_t>  rem(1, i);
            size_t el = reorder(rem(generator), order, seed);
            auto it = set.find(el);
            if(it != set.end())
                set.erase(it);
        }
    }
    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "COMPLETED IN " << (0.001*elapsed.count()) << " SECONDS " << std::endl;
}

void set_insert_ptrie(auto& set, size_t elements, size_t seed, double deletes, double read_rate, std::vector<size_t>& order)
{
    std::default_random_engine del_generator(seed);
    std::uniform_real_distribution<double> del_dist;

    std::default_random_engine read_generator(seed);
    std::normal_distribution<double> read_dist(read_rate, read_rate / 2.0);
    std::uniform_int_distribution<size_t> read_el(1, elements);
    auto start = std::chrono::system_clock::now();

    for(size_t i = 1; i <= elements; ++i)
    {
        size_t val = reorder(i, order, seed);
        set.insert((unsigned char*)&val, sizeof(val));

        if(read_rate > 0.0)
        {
            int reads = std::round(read_dist(read_generator));
            for(int r = 0; r < reads; ++r)
            {
                size_t el = reorder(read_el(read_generator), order, seed);
                set.exists((unsigned char*)&el, sizeof(el));
            }
        }
        if(del_dist(del_generator) < deletes)
        {

            std::uniform_int_distribution<size_t>  del_el(1, elements);
            size_t el = reorder(del_el(del_generator), order, seed);
            set.erase((unsigned char*)&el, sizeof(el));
        }
    }
    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "COMPLETED IN " << (0.001*elapsed.count()) << " SECONDS " << std::endl;
}


struct hasher_o
{
    uint64_t operator()(const size_t& w) const
    {
        return MurmurHash64A(&w, sizeof(size_t), 0);
    }
};

struct equal_o
{
    bool operator()(const size_t& w1, const size_t& w2) const
    {
        return w1 == w2;
    }
};

int main(int argc, const char** argv)
{
    if(argc < 3 || argc > 8)
    {
        std::cout << "usage : <ptrie/std/sparse/dense> <number elements> <?seed> <?delete ratio> <?read rate>" << std::endl;
        exit(-1);
    }

    const char* type = argv[1];
    size_t elements = 1024;
    size_t seed = 0;
    double deletes = 0.0;
    double read_rate = 0.0;

    read_arg(argv[2], elements, "Error in <number of elements>", "%zu");
    if(argc > 3) read_arg(argv[3], seed, "Error in <seed>", "%zu");
    if(argc > 4) read_arg(argv[4], deletes, "Error in <delete ratio>", "%lf");
    if(argc > 5) read_arg(argv[5], read_rate, "Error in <read rate>", "%lf");

    std::vector<size_t> order;
    for(size_t i = 0; i < sizeof(size_t)*8; ++i)
    {
        order.push_back(i);
    }

    std::srand(seed);
    std::random_shuffle(order.begin(), order.end());


    if(strcmp(type, "ptrie") == 0)
    {
        print_settings(type, elements, seed, sizeof(size_t), deletes, read_rate, 256);
        ptrie::set<> set;
        set_insert_ptrie(set, elements, std::rand(), deletes, read_rate, order);
    }
    else if (strcmp(type, "std") == 0) {
        print_settings(type, elements, seed, sizeof(size_t), deletes, read_rate, 256);
        std::unordered_set<size_t, hasher_o, equal_o> set;
        set_insert(set, elements, std::rand(), deletes, read_rate, order);
    }
    else if(strcmp(type, "redblack") == 0)
    {
        print_settings(type, elements, seed, sizeof(size_t), deletes, read_rate, 256);
        std::set<size_t> set;
        set_insert(set, elements, std::rand(), deletes, read_rate, order);
    }
    /*else if(strcmp(type, "tbb") == 0)
    {
        print_settings(type, elements, seed, sizeof(size_t), deletes, read_rate, 256);
        tbb::concurrent_unordered_set<size_t, hasher_o, equal_o> set;
        set_insert(set, elements, std::rand(), deletes, read_rate);
    }*/
    else if(strcmp(type, "sparse") == 0)
    {
        print_settings(type, elements, seed, sizeof(size_t), deletes, read_rate, 256);
        google::sparse_hash_set<size_t , hasher_o, equal_o> set(elements/10);
        set_insert(set, elements, std::rand(), deletes, read_rate, order);
    }
    else if(strcmp(type, "dense") == 0)
    {
        print_settings(type, elements, seed, sizeof(size_t), deletes, read_rate, 256);
        seed = std::rand();
        google::dense_hash_set<size_t , hasher_o, equal_o> set(elements/10);
        set.set_empty_key(reorder(0, order, seed));
        if(deletes > 0.0) set.set_deleted_key(reorder(std::numeric_limits<uint32_t>::max(), order, seed));
        set_insert(set, elements, seed , deletes, read_rate, order);
    }
    else
    {
        std::cerr << "ERROR IN TYPE, ONLY VALUES ALLOWED : ptrie, std, sparse, dense, tbb" << std::endl;
        exit(-1);
    }

    return 0;
}

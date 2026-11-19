// Host-side fakes implementing the HAL for simulation/testing.
#pragma once
#include "hal.h"
#include <vector>
#include <deque>

struct SimLeds : hal::ILeds {
    std::vector<hw::RGB> buf;
    int shows=0;
    SimLeds():buf(hw::NLED,{0,0,0}){}
    void set(int i, hw::RGB c) override { if(i>=0&&i<(int)buf.size()) buf[i]=c; }
    void clear() override { for(auto& c:buf) c={0,0,0}; }
    void show() override { shows++; }
    int count() const override { return (int)buf.size(); }
    int litCount() const { int n=0; for(auto&c:buf) if(c.r||c.g||c.b) n++; return n; }
};

struct SimTime : hal::ITime {
    long t=0;
    long now() override { return t; }
    void advance(long ms){ t+=ms; }
};

struct SimMatrix : hal::IMatrix {
    std::deque<std::pair<int,int>> q;
    void push(int r,int c){ q.push_back({r,c}); }
    bool nextPress(int& r,int& c) override {
        if(q.empty()) return false;
        r=q.front().first; c=q.front().second; q.pop_front(); return true;
    }
};

#pragma once
// inspired by https://stackoverflow.com/a/64351208/1636521
template<typename IntType = int>
class IntRange {
    public:
    // TODO
    // what if step is negative? then the range will be infinite. 
    // do i check in the constructor for this condition and generate
    // and exception or i just create two separete classes
    // one going forward and another going backward
    // or if the step is negative is just swap start and stop?
    // for now since i know i will be going only forward leave it as is
    // but generelizing this could be a good idea
    IntRange(IntType start, IntType stop, IntType step) :
        curr(start), stop(stop), step(step) {}
    auto begin() { return *this; }
    auto end() { return *this; }

    auto operator*() const { return curr; }
    auto& operator++() 
    { 
        curr += step;
        return *this;
    }

    bool operator!=(const IntRange &rhs) const
    {
        return curr < rhs.stop;
    }

    private:
    IntType curr, stop, step;
};

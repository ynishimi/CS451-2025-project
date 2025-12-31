#pragma once
#include <iostream>

#include <set>
#include <string>
#include <algorithm>
#include <vector>

using namespace std;

// print set<T>
template <typename T>
std::ostream &operator<<(std::ostream &out, const std::set<T> &set)
{
    if (set.empty())
        return out << "";
    out << *set.begin();
    std::for_each(std::next(set.begin()), set.end(), [&out](const T &element)
                  { out << " " << element; });
    return out << "";
}

template <typename T>
void debugPrint(const string &label, const T &debug_data)
{
    // cout << "[DEBUG] " << label << ": " << debug_data << endl;
}

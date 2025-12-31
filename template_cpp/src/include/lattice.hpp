#pragma once
#include <set>
#include <functional>
#include <iostream>
#include <sstream>
#include "common.hpp"

using namespace std;

typedef std::set<int> proposalSet;

enum class LatticeMessageType
{
    PROPOSAL,
    ACK,
    NACK,
};
inline string enum_to_string(LatticeMessageType type)
{
    return to_string(static_cast<int>(type));
}

// data used in Lattice Agreement
struct LatticePayload
{
    LatticeMessageType type;
    proposalSet proposed_value;
    unsigned int active_proposal_number;

    // PROPOSAL:{1,2,3,}:1
    string serialize() const
    {
        string res = "";
        res += enum_to_string(type) + ":";
        bool first = true;

        for (auto &val : proposed_value)
        {
            if (!first)
                res += ",";
            res += to_string(val);
            first = false;
        }
        res += ":" + to_string(active_proposal_number);

        // cout << "serialize: " << res << endl;

        return res;
    };

    void deserialize(string payload_m)
    {
        // debugPrint("deserialize", payload_m);
        // type
        auto delim1 = payload_m.find(':');
        // cout << "payload:" << payload_m.substr(0, delim1) << endl;
        this->type = static_cast<LatticeMessageType>(stoi(payload_m.substr(0, delim1)));

        string remaining_str = payload_m.substr(delim1 + 1);

        // proposed_value (e.g., 1,2,3,)
        auto delim2 = remaining_str.find(':');
        string proposed_value_string = remaining_str.substr(0, delim2);
        if (proposed_value_string != "")
        {
            istringstream ss(proposed_value_string);
            string t;
            while (getline(ss, t, ','))
            {
                proposed_value.insert(stoi(t));
            }
        }

        // active_proposal_number
        active_proposal_number = stoi(remaining_str.substr(delim2 + 1));
    };
};
inline ostream &operator<<(ostream &os, const LatticePayload &payload)
{
    os << "LatticePayload{";
    os << "type: " << static_cast<int>(payload.type) << ", proposed_value: " << payload.proposed_value << ", active_proposal_number: " << payload.active_proposal_number;
    os << "}";
    return os;
}

class LatticeProposer
{
public:
    LatticeProposer(int lattice_shot_num, int f, function<void(const LatticePayload &)> broadcastCallback, function<void(const proposalSet &)> decideCallback)
        : lattice_shot_num_(lattice_shot_num), f_(f), broadcastPayloadCallback_(broadcastCallback), decideCallback_(decideCallback)
    {
        Init();
    }
    void Init();
    void Propose(proposalSet proposal);
    void Receive(const LatticePayload &p);

private:
    bool active_;
    unsigned int ack_count_;
    unsigned int nack_count_;
    // active_proposal_number_ denotes the number used to distinguish between proposals WITHIN this shot (a single line of proposal in config)
    unsigned int active_proposal_number_;
    proposalSet proposed_value_;

    int lattice_shot_num_;
    // n = 2f + 1
    unsigned int f_;

    // broadcast (using broadcastCallback)
    function<void(const LatticePayload &)> broadcastPayloadCallback_;
    // update Peer's decision
    function<void(const proposalSet &)> decideCallback_;
};

class LatticeAcceptor
{
public:
    LatticeAcceptor(int lattice_shot_num, function<void(unsigned long src_id, const LatticePayload &)> sendCallback)
        : lattice_shot_num_(lattice_shot_num), sendPayloadCallback_(sendCallback)
    {
        Init();
    }
    void Init();
    void Receive(unsigned long src_id, const LatticePayload &p);

private:
    proposalSet accepted_value_;
    int lattice_shot_num_;
    function<void(unsigned long src_id, LatticePayload)> sendPayloadCallback_;
};

bool isProposalSetIncluded(proposalSet base, proposalSet compare);
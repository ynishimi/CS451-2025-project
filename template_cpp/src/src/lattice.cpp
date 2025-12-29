#include "lattice.hpp"

// LatticeProposer

// Init initializes variables.
void LatticeProposer::Init()
{
    active_ = false;
    ack_count_ = 0;
    nack_count_ = 0;
    active_proposal_number_ = 0;
    proposed_value_ = {};
}

// Propose proposes a single proposal.
void LatticeProposer::Propose(proposalSet proposal)
{
    proposed_value_ = proposal;
    active_ = true;
    // assign new number to the propose
    active_proposal_number_++;
    ack_count_ = 0;
    nack_count_ = 0;

    // broadcast
    // message should contain the following data:
    // - message type (proposal, ack, nack) in higher level
    // - proposed_value (proposal)
    // - active_proposal_number (proposal shot)
    broadcastPayloadCallback_(lattice_shot_num_, {LatticeMessageType::PROPOSAL, proposed_value_, active_proposal_number_});
}

// Receive processes ack or nack
void LatticeProposer::Receive(const LatticePayload &p)
{
    if (p.active_proposal_number != active_proposal_number_)
    {
        // should not reach
        return;
    }

    switch (p.type)
    {
    case LatticeMessageType::ACK:
        ack_count_++;
        break;

    case LatticeMessageType::NACK:
        proposed_value_.insert(p.proposed_value.begin(), p.proposed_value.end());
        nack_count_++;
        break;

    default:
        // should not reach
    }

    if (nack_count_ > 0 && (ack_count_ + nack_count_) >= (f_ + 1) && active_)
    {
        active_proposal_number_++;
        ack_count_ = 0;
        nack_count_ = 0;

        // broadcast updated values (since NACK is received)
        broadcastPayloadCallback_(lattice_shot_num_, {LatticeMessageType::PROPOSAL, proposed_value_, active_proposal_number_});
    }

    if (ack_count_ > f_ + 1 && active_)
    {
        active_ = false;
    }
}

// LatticeAcceptor
void LatticeAcceptor::Init()
{
    accepted_value_ = {};
}
void LatticeAcceptor::Receive(unsigned long src_id, const LatticePayload &p)
{
    if (p.type == LatticeMessageType::PROPOSAL)
    {
        if (isProposalSetIncluded(accepted_value_, p.proposed_value))
        {
            // proposal includes my set: ack
            accepted_value_ = p.proposed_value;
            sendPayloadCallback_(src_id, lattice_shot_num_, LatticePayload{LatticeMessageType::ACK, {}, p.active_proposal_number});
        }
        else
        {
            // proposal is missing some part of my set: nack and send corrected proposal
            accepted_value_.insert(p.proposed_value.begin(), p.proposed_value.end());
            sendPayloadCallback_(src_id, lattice_shot_num_, LatticePayload{LatticeMessageType::NACK, accepted_value_, p.active_proposal_number});
        }
    }
}

// judges if base is included to compare
bool isProposalSetIncluded(proposalSet base, proposalSet compare)
{
    for (auto &v : base)
    {
        if (compare.find(v) == compare.end())
        {
            return false;
        }
    }
    return true;
}
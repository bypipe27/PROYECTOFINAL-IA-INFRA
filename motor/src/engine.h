#pragma once

#include "board.h"

#include <cstdint>

class Engine {
public:
    struct Result {
        int best_move = -1;
        double evaluation = 0.0;
        std::uint64_t nodes_visited = 0;
        std::uint64_t prunes = 0;
    };

    explicit Engine(double heuristic_weight = 0.1);

    Result minimax(const Board& board, Board::Player player, int depth) const;
    Result alpha_beta(const Board& board, Board::Player player, int depth, int threads = 1) const;

private:
    double heuristic_weight_;

    double evaluate(const Board& board, Board::Player player) const;

    double minimax_search(const Board& board,
                          Board::Player to_move,
                          Board::Player root_player,
                          int depth,
                          std::uint64_t& nodes) const;

    double alpha_beta_search(const Board& board,
                            Board::Player to_move,
                            Board::Player root_player,
                            int depth,
                            double alpha,
                            double beta,
                            std::uint64_t& nodes,
                            std::uint64_t& prunes) const;

    bool is_maximizing(Board::Player current, Board::Player root) const;
};
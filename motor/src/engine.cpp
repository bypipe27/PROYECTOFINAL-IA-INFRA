#include "engine.h"

#include <algorithm>
#include <limits>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace {
constexpr double kNegativeInfinity = -std::numeric_limits<double>::infinity();
constexpr double kPositiveInfinity = std::numeric_limits<double>::infinity();

bool is_better_score(double candidate, double current, bool maximizing) {
    if (maximizing) {
        return candidate > current;
    }
    return candidate < current;
}
}  // namespace

Engine::Engine(double heuristic_weight) : heuristic_weight_(heuristic_weight) {}

Engine::Result Engine::minimax(const Board& board, Board::Player player, int depth) const {
    Result result;
    const auto moves = board.legal_moves(player);
    if (moves.empty()) {
        result.evaluation = evaluate(board, player);
        return result;
    }

    const bool maximizing_root = true;
    double best_score = maximizing_root ? kNegativeInfinity : kPositiveInfinity;
    int best_move = moves.front();

    for (int move : moves) {
        bool extra_turn = false;
        Board next = board.apply_move(move, player, &extra_turn);
        Board::Player next_player = extra_turn ? player : (player == Board::Player::Player1 ? Board::Player::Player2 : Board::Player::Player1);

        std::uint64_t branch_nodes = 1;
        const double score = minimax_search(next, next_player, player, depth - 1, branch_nodes);
        result.nodes_visited += branch_nodes;

        if (is_better_score(score, best_score, maximizing_root)) {
            best_score = score;
            best_move = move;
        }
    }

    result.best_move = best_move;
    result.evaluation = best_score;
    return result;
}

Engine::Result Engine::alpha_beta(const Board& board, Board::Player player, int depth, int threads) const {
    Result result;
    const auto moves = board.legal_moves(player);
    if (moves.empty()) {
        result.evaluation = evaluate(board, player);
        return result;
    }

    if (threads < 1) {
        threads = 1;
    }

    const bool maximizing_root = true;
    double best_score = maximizing_root ? kNegativeInfinity : kPositiveInfinity;
    int best_move = moves.front();

    if (threads == 1 || moves.size() == 1) {
        for (int move : moves) {
            bool extra_turn = false;
            Board next = board.apply_move(move, player, &extra_turn);
            Board::Player next_player = extra_turn ? player : (player == Board::Player::Player1 ? Board::Player::Player2 : Board::Player::Player1);

            std::uint64_t branch_nodes = 1;
            std::uint64_t branch_prunes = 0;
            const double score = alpha_beta_search(next, next_player, player, depth - 1, kNegativeInfinity, kPositiveInfinity, branch_nodes, branch_prunes);
            result.nodes_visited += branch_nodes;
            result.prunes += branch_prunes;

            if (is_better_score(score, best_score, maximizing_root)) {
                best_score = score;
                best_move = move;
            }
        }

        result.best_move = best_move;
        result.evaluation = best_score;
        return result;
    }

    std::uint64_t total_nodes = 0;
    std::uint64_t total_prunes = 0;

    struct BranchResult {
        int move = -1;
        double score = kNegativeInfinity;
        std::uint64_t nodes = 0;
        std::uint64_t prunes = 0;
    };

    std::vector<BranchResult> branch_results(moves.size());

#pragma omp parallel for num_threads(threads) schedule(static)
    for (int i = 0; i < static_cast<int>(moves.size()); ++i) {
        const int move = moves[static_cast<std::size_t>(i)];
        bool extra_turn = false;
        Board next = board.apply_move(move, player, &extra_turn);
        Board::Player next_player = extra_turn ? player : (player == Board::Player::Player1 ? Board::Player::Player2 : Board::Player::Player1);

        std::uint64_t branch_nodes = 1;
        std::uint64_t branch_prunes = 0;
        const double score = alpha_beta_search(next, next_player, player, depth - 1, kNegativeInfinity, kPositiveInfinity, branch_nodes, branch_prunes);

        branch_results[static_cast<std::size_t>(i)] = BranchResult{move, score, branch_nodes, branch_prunes};
    }

    for (const BranchResult& branch : branch_results) {
        total_nodes += branch.nodes;
        total_prunes += branch.prunes;
        if (is_better_score(branch.score, best_score, maximizing_root)) {
            best_score = branch.score;
            best_move = branch.move;
        }
    }

    result.best_move = best_move;
    result.evaluation = best_score;
    result.nodes_visited = total_nodes;
    result.prunes = total_prunes;
    return result;
}

double Engine::evaluate(const Board& board, Board::Player player) const {
    const Board::Player opponent = (player == Board::Player::Player1) ? Board::Player::Player2 : Board::Player::Player1;
    const double own_store = static_cast<double>(board.store(player));
    const double opp_store = static_cast<double>(board.store(opponent));
    const double own_seeds = static_cast<double>(board.pit_count(player));
    const double opp_seeds = static_cast<double>(board.pit_count(opponent));
    return (own_store - opp_store) + heuristic_weight_ * (own_seeds - opp_seeds);
}

double Engine::minimax_search(const Board& board,
                              Board::Player to_move,
                              Board::Player root_player,
                              int depth,
                              std::uint64_t& nodes) const {
    ++nodes;

    if (depth <= 0 || board.is_terminal()) {
        return evaluate(board, root_player);
    }

    const auto moves = board.legal_moves(to_move);
    if (moves.empty()) {
        return evaluate(board, root_player);
    }

    const bool maximizing = is_maximizing(to_move, root_player);
    double best_score = maximizing ? kNegativeInfinity : kPositiveInfinity;

    for (int move : moves) {
        bool extra_turn = false;
        Board next = board.apply_move(move, to_move, &extra_turn);
        Board::Player next_player = extra_turn ? to_move : (to_move == Board::Player::Player1 ? Board::Player::Player2 : Board::Player::Player1);
        const double score = minimax_search(next, next_player, root_player, depth - 1, nodes);
        if (is_better_score(score, best_score, maximizing)) {
            best_score = score;
        }
    }

    return best_score;
}

double Engine::alpha_beta_search(const Board& board,
                                 Board::Player to_move,
                                 Board::Player root_player,
                                 int depth,
                                 double alpha,
                                 double beta,
                                 std::uint64_t& nodes,
                                 std::uint64_t& prunes) const {
    ++nodes;

    if (depth <= 0 || board.is_terminal()) {
        return evaluate(board, root_player);
    }

    const auto moves = board.legal_moves(to_move);
    if (moves.empty()) {
        return evaluate(board, root_player);
    }

    const bool maximizing = is_maximizing(to_move, root_player);

    if (maximizing) {
        double best_score = kNegativeInfinity;
        for (int move : moves) {
            bool extra_turn = false;
            Board next = board.apply_move(move, to_move, &extra_turn);
            Board::Player next_player = extra_turn ? to_move : (to_move == Board::Player::Player1 ? Board::Player::Player2 : Board::Player::Player1);
            const double score = alpha_beta_search(next, next_player, root_player, depth - 1, alpha, beta, nodes, prunes);
            best_score = std::max(best_score, score);
            alpha = std::max(alpha, best_score);
            if (alpha >= beta) {
                ++prunes;
                break;
            }
        }
        return best_score;
    }

    double best_score = kPositiveInfinity;
    for (int move : moves) {
        bool extra_turn = false;
        Board next = board.apply_move(move, to_move, &extra_turn);
        Board::Player next_player = extra_turn ? to_move : (to_move == Board::Player::Player1 ? Board::Player::Player2 : Board::Player::Player1);
        const double score = alpha_beta_search(next, next_player, root_player, depth - 1, alpha, beta, nodes, prunes);
        best_score = std::min(best_score, score);
        beta = std::min(beta, best_score);
        if (alpha >= beta) {
            ++prunes;
            break;
        }
    }
    return best_score;
}

bool Engine::is_maximizing(Board::Player current, Board::Player root) const {
    return current == root;
}
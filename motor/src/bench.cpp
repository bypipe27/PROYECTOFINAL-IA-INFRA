#include "board.h"
#include "engine.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
struct Position {
    Board::State state{};
};

void print_usage(const char* program) {
    std::cerr << "Usage: " << program << " --depth N --positions path/to/suite.txt\n";
}

bool parse_args(int argc, char** argv, int& depth, std::string& positions_path) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--depth") {
            if (i + 1 >= argc) {
                return false;
            }
            depth = std::stoi(argv[++i]);
        } else if (arg == "--positions") {
            if (i + 1 >= argc) {
                return false;
            }
            positions_path = argv[++i];
        } else {
            return false;
        }
    }

    return depth > 0 && !positions_path.empty();
}

std::vector<Position> load_positions(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open positions file: " + path);
    }

    std::vector<Position> positions;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream stream(line);
        Position position;
        for (int i = 0; i < Board::kBoardSize; ++i) {
            if (!(stream >> position.state[static_cast<std::size_t>(i)])) {
                throw std::runtime_error("invalid position line: " + line);
            }
        }

        int extra = 0;
        if (stream >> extra) {
            throw std::runtime_error("position line has more than 14 integers: " + line);
        }

        positions.push_back(position);
    }

    return positions;
}

Board::Player infer_player_to_move(const Board& board) {
    const bool player1_empty = board.pit_count(Board::Player::Player1) == 0;
    const bool player2_empty = board.pit_count(Board::Player::Player2) == 0;
    if (player1_empty && !player2_empty) {
        return Board::Player::Player2;
    }
    return Board::Player::Player1;
}

std::string player_name(Board::Player player) {
    return player == Board::Player::Player1 ? "P1" : "P2";
}
}  // namespace

int main(int argc, char** argv) {
    int depth = 0;
    std::string positions_path;

    if (!parse_args(argc, argv, depth, positions_path)) {
        print_usage(argv[0]);
        return 1;
    }

    int threads = 1;
    if (const char* env_threads = std::getenv("OMP_NUM_THREADS")) {
        const int parsed = std::atoi(env_threads);
        if (parsed > 0) {
            threads = parsed;
        }
    }

    std::vector<Position> positions;
    try {
        positions = load_positions(positions_path);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    Engine engine;

    std::cout << std::left
              << std::setw(6) << "idx"
              << std::setw(4) << "pl"
              << std::setw(12) << "min_move"
              << std::setw(12) << "ab_move"
              << std::setw(14) << "min_nodes"
              << std::setw(14) << "ab_nodes"
              << std::setw(12) << "prunes"
              << std::setw(14) << "min_ms"
              << std::setw(14) << "ab_ms"
              << std::setw(8) << "ok"
              << '\n';

    for (std::size_t index = 0; index < positions.size(); ++index) {
        const Board board(positions[index].state);
        const Board::Player player = infer_player_to_move(board);

        const auto min_start = std::chrono::steady_clock::now();
        const Engine::Result minimax_result = engine.minimax(board, player, depth);
        const auto min_end = std::chrono::steady_clock::now();

        const auto ab_start = std::chrono::steady_clock::now();
        const Engine::Result alpha_beta_result = engine.alpha_beta(board, player, depth, threads);
        const auto ab_end = std::chrono::steady_clock::now();

        const auto min_ms = std::chrono::duration_cast<std::chrono::milliseconds>(min_end - min_start).count();
        const auto ab_ms = std::chrono::duration_cast<std::chrono::milliseconds>(ab_end - ab_start).count();

        const bool same_move = minimax_result.best_move == alpha_beta_result.best_move;
        if (!same_move) {
            std::cerr << "Mismatch at position " << index << ": minimax=" << minimax_result.best_move
                      << " alpha_beta=" << alpha_beta_result.best_move << '\n';
            return 2;
        }

        std::cout << std::left
                  << std::setw(6) << index
                  << std::setw(4) << player_name(player)
                  << std::setw(12) << minimax_result.best_move
                  << std::setw(12) << alpha_beta_result.best_move
                  << std::setw(14) << minimax_result.nodes_visited
                  << std::setw(14) << alpha_beta_result.nodes_visited
                  << std::setw(12) << alpha_beta_result.prunes
                  << std::setw(14) << min_ms
                  << std::setw(14) << ab_ms
                  << std::setw(8) << "yes"
                  << '\n';
    }

    return 0;
}
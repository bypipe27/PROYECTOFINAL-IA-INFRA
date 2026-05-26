#include "board.h"

#include <algorithm>
#include <stdexcept>

Board::Board() {
	state_.fill(4);
	state_[kPlayer1Store] = 0;
	state_[kPlayer2Store] = 0;
}

Board::Board(const State& state) : state_(state) {}

Board Board::initial() {
	return Board();
}

const Board::State& Board::state() const {
	return state_;
}

int Board::operator[](std::size_t index) const {
	return state_.at(index);
}

int& Board::operator[](std::size_t index) {
	return state_.at(index);
}

std::vector<int> Board::legal_moves(Player player) const {
	std::vector<int> moves;
	const int start = (player == Player::Player1) ? kPlayer1PitsStart : kPlayer2PitsStart;
	for (int offset = 0; offset < kPitsPerSide; ++offset) {
		const int index = start + offset;
		if (state_[index] > 0) {
			moves.push_back(index);
		}
	}
	return moves;
}

Board Board::apply_move(int pit_index, Player player, bool* extra_turn) const {
	validate_move(pit_index, player);

	Board next(*this);
	const int own_store = store_index(player);
	const int opponent_store = store_index(player == Player::Player1 ? Player::Player2 : Player::Player1);

	int seeds = next.state_[pit_index];
	next.state_[pit_index] = 0;

	int current = pit_index;
	while (seeds > 0) {
		current = next_index(current);
		if (current == opponent_store) {
			current = next_index(current);
		}
		++next.state_[current];
		--seeds;
	}

	bool grants_extra_turn = current == own_store;

	if (!grants_extra_turn && is_player_pit(player, current) && next.state_[current] == 1) {
		const int opposite = opposite_index(current);
		if (next.state_[opposite] > 0) {
			next.state_[own_store] += next.state_[opposite] + next.state_[current];
			next.state_[opposite] = 0;
			next.state_[current] = 0;
		}
	}

	if (next.is_terminal()) {
		next.collect_remaining_seeds();
	}

	if (extra_turn != nullptr) {
		*extra_turn = grants_extra_turn;
	}

	return next;
}

bool Board::is_terminal() const {
	const bool player1_empty = pit_count(Player::Player1) == 0;
	const bool player2_empty = pit_count(Player::Player2) == 0;
	return player1_empty || player2_empty;
}

void Board::collect_remaining_seeds() {
	const int player1_remaining = pit_count(Player::Player1);
	const int player2_remaining = pit_count(Player::Player2);

	for (int offset = 0; offset < kPitsPerSide; ++offset) {
		state_[kPlayer1PitsStart + offset] = 0;
		state_[kPlayer2PitsStart + offset] = 0;
	}

	state_[kPlayer1Store] += player1_remaining;
	state_[kPlayer2Store] += player2_remaining;
}

int Board::store(Player player) const {
	return state_[store_index(player)];
}

int Board::pit_count(Player player) const {
	const int start = (player == Player::Player1) ? kPlayer1PitsStart : kPlayer2PitsStart;
	int total = 0;
	for (int offset = 0; offset < kPitsPerSide; ++offset) {
		total += state_[start + offset];
	}
	return total;
}

bool Board::is_player_pit(Player player, int index) {
	if (player == Player::Player1) {
		return index >= kPlayer1PitsStart && index < kPlayer1PitsStart + kPitsPerSide;
	}
	return index >= kPlayer2PitsStart && index < kPlayer2PitsStart + kPitsPerSide;
}

int Board::store_index(Player player) {
	return player == Player::Player1 ? kPlayer1Store : kPlayer2Store;
}

int Board::opposite_index(int pit_index) {
	return 12 - pit_index;
}

int Board::next_index(int index) {
	return (index + 1) % kBoardSize;
}

void Board::validate_move(int pit_index, Player player) const {
	if (pit_index < 0 || pit_index >= kBoardSize) {
		throw std::out_of_range("pit index out of range");
	}
	if (!is_player_pit(player, pit_index)) {
		throw std::invalid_argument("pit does not belong to the player");
	}
	if (state_[pit_index] <= 0) {
		throw std::invalid_argument("pit is empty");
	}
}

#pragma once

#include <array>
#include <cstddef>
#include <vector>

class Board {
public:
	static constexpr int kBoardSize = 14;
	static constexpr int kPlayer1PitsStart = 0;
	static constexpr int kPlayer1Store = 6;
	static constexpr int kPlayer2PitsStart = 7;
	static constexpr int kPlayer2Store = 13;
	static constexpr int kPitsPerSide = 6;

	using State = std::array<int, kBoardSize>;

	enum class Player {
		Player1,
		Player2,
	};

	Board();
	explicit Board(const State& state);

	static Board initial();

	const State& state() const;
	int operator[](std::size_t index) const;
	int& operator[](std::size_t index);

	std::vector<int> legal_moves(Player player) const;
	Board apply_move(int pit_index, Player player, bool* extra_turn = nullptr) const;
	bool is_terminal() const;
	void collect_remaining_seeds();

	int store(Player player) const;
	int pit_count(Player player) const;

private:
	State state_{};

	static bool is_player_pit(Player player, int index);
	static int store_index(Player player);
	static int opposite_index(int pit_index);
	static int next_index(int index);
	void validate_move(int pit_index, Player player) const;
};

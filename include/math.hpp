#ifndef INC_MATH_HPP_
#define INC_MATH_HPP_

inline long long round_down(long long price, long long mini_tick)
{
	long long mod = price % mini_tick;
	if (mod >= 0)
		return price - mod;
	else
		return price - mod - mini_tick;
}

inline long long round_up(long long price, long long mini_tick)
{
	long long mod = price % mini_tick;
	if (mod == 0)
		return price;
	if (mod > 0)
		return price + mini_tick - mod;
	else
		return price - mod;
}

inline long long handle_pips(long long price, long long pips)
{
	auto commission = price;
	if (commission < 0)
		commission = 0 - commission;
	commission *= pips;
	commission /= 10000;
	return commission;
}


#endif /* INC_MATH_HPP_ */

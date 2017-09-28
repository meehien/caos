#ifndef CSWCaosCLIENT_BUFFERMAPNAMES_HPP
#define CSWCaosCLIENT_BUFFERMAPNAMES_HPP

struct BufferMap {
	std::vector<int64_t> bids;
	std::vector<int64_t> pos;
};

constexpr auto bufferdata = "buffer.bin";
constexpr auto buffermap = "buffer.map";

#endif //CSWCaosCLIENT_BUFFERMAPNAMES_HPP

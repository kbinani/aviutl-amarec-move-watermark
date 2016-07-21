#pragma once

struct MaskPixel
{
	MaskPixel(int32_t r, int32_t g, int32_t b)
		: ir(r)
		, ig(g)
		, ib(b)
	{
		static_assert(sizeof(int32_t) == sizeof(float), "");
		static_assert(sizeof(MaskPixel) == 3 * sizeof(int32_t), "");
	}

	union {
		int32_t const ir;
		float const fr;
	};
	union {
		int32_t const ig;
		float const fg;
	};
	union {
		int32_t const ib;
		float const fb;
	};
};

#pragma once
const UINT SWAP_CHAIN_FRAME_COUNT = 3;

// Pending : "보류중인" 이라는 뜻
// 화면에 그려지는 전면버퍼 하나 빼고 나머지 후면버퍼들의 갯수를 말함
const UINT MAX_PENDING_FRAME_COUNT = SWAP_CHAIN_FRAME_COUNT - 1;

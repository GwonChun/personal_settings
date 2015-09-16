typedef struct mt_struct_s {
    uint aaa;
    int mm, nn, rr, ww;
    uint wmask, umask, lmask;
    int shift0, shift1, shiftB, shiftC;
    uint maskB, maskC;
} mt_struct ;

/* seed를 이용하여 상태 초기화 */
void sgenrand_mt(uint seed, __local const mt_struct *mts, __local uint *state) {
    int i;
    for(i = 0; i <mts -> nn; i++) {
	state[i] = seed;
        seed = (1812433253 * (seed  ^ (seed >> 30))) + i + 1;
    }
    for(i = 0; i < mts -> nn; i++)
	state[i] &= mts -> wmask;
}

/* 상태 갱신 */
void update_state(__local const mt_struct *mts, __local uint *st) {
    int n = mts -> nn, m = mts -> mm;
    uint aa = mts -> aaa, x;
    uint uuu = mts -> umask, lll = mts -> lmask;
    int k, lim;
    lim = n - m;
    for(k = 0; k < lim; k++) {
        x = (st[k]&uuu) | (st[k+1]&lll);
        st[k] = st[k+m] ^ (x >> 1) ^ (x&1U ? aa : 0U);
    }
    lim = n - 1;
    for(; k < lim; k++) {
        x = (st[k]&uuu) | (st[k+1]&lll);
        st[k] = st[k+m-n] ^ (x >> 1) ^ (x&1U ? aa : 0U);
    }
    x = (st[n-1]&uuu) | (st[0]&lll);
    st[n-1] = st[m-1] ^ (x >> 1) ^ (x&1U ? aa : 0U);
}

inline void gen(__global uint *out, const __local mt_struct *mts, __local uint *state, int num_rand) {
    int i, j, n, nn = mts -> nn;
    n = (num_rand+(nn-1)) / nn;
    for(i = 0; i < n; i++) {
        int m = nn;
        if(i == n-1) m = num_rand%nn;
        update_state(mts, state);

        for(j = 0; j < m; j++) {
            /* 난수 생성 */
            uint x = state[j];
            x ^= x >> mts -> shift0;
            x ^= (x << mts -> shiftB) & mts -> maskB;
            x ^= (x << mts->shiftC) & mts -> maskC;
            x ^= x >> mts -> shift1;
            out[i*nn + j] = x;
        }
    }
}

__kernel void genrand(__global uint *out,__global mt_struct *mts_g,int num_rand, int num_generator, __local uint *state_mem, __local mt_struct *mts) {
    int lid = get_local_id(0);
    int gid = get_global_id(0);
    __local uint *state = state_mem + lid*17; /* 상태를 로컬 메모리에 저장 */

    if(gid >= num_generator) return;

    mts += lid;

    /* 파라메터를 로컬 메모리에 복사 */
    mts -> aaa = mts_g[gid].aaa;
    mts -> mm = mts_g[gid].mm;
    mts -> nn = mts_g[gid].nn;
    mts -> rr = mts_g[gid].rr;
    mts -> ww = mts_g[gid].ww;
    mts -> wmask = mts_g[gid].wmask;
    mts -> umask = mts_g[gid].umask;
    mts -> lmask = mts_g[gid].lmask;
    mts -> shift0 = mts_g[gid].shift0;
    mts -> shift1 = mts_g[gid].shift1;
    mts -> shiftB = mts_g[gid].shiftB;
    mts -> shiftC = mts_g[gid].shiftC;
    mts -> maskB = mts_g[gid].maskB;
    mts -> maskC = mts_g[gid].maskC;

    out += gid * num_rand;      /* 이 아이템에 해당하는 출력 버퍼 */
    sgenrand_mt(0x33ff*gid, mts, (__local uint*)state); /* 난수 초기화 */
    gen(out, mts, (__local uint*)state, num_rand);      /* 난수 생성 */
}

/* 난수열중에 원에 포함되는 점의 갯수를 계산 */
__kernel void calc_pi(__global uint *out, __global uint *rand, int num_rand, int num_generator) {
    int gid = get_global_id(0);
    int count = 0;
    int i;

    if(gid >= num_generator) return;

    rand += gid*num_rand;

    for(i = 0; i < num_rand; i++) {
        float x, y, len;
        x = ((float)(rand[i] >> 16)) / 65535.0f;    /* x좌표 */
        y = ((float)(rand[i]&0xffff)) / 65535.0f; /* y좌표 */
        len = (x*x + y*y);      /* 원점에서 난수가 나타내는 점까지의 거리 */

        if(len < 1) {          /* sqrt(len) < 1 = len < 1 */
           count++;
        }
    }

    out[gid] = count;
}
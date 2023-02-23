kernel void store_uniform_pointer(__global int *out, int zero) {
	int value = get_global_id(0) * zero + 7;
	out[3] = value;
}

p_max_terminate_counter       =   10
p_thr_messed_up_kickin        =   0
p_thr_stuck_kickin            =   1e5
p_max_messed_up               =   10
p_max_stuck                   =   40
p_messed_up_factor            =   0.5
p_lim_times                   =   10
p_consecutive_success         =   3
p_thr_messed_up_switch        =   5
p_messed_lim_times            =   20
p_messed_consecutive_success  =   6

p_gen_cex_lim_leaves          =   16
p_gen_cex_heavy               =   [5, 5, 5]
p_gen_cex_normal              =   [5, 0, 0]
p_gen_cex_retry               =   [0, 5, 0]


if (config_space < 1e5) {
    p_max_terminate_counter       =   3

    p_lim_times                   =   6
    p_consecutive_success         =   2
    p_thr_messed_up_switch        =   3
    p_messed_lim_times            =   10
    p_messed_consecutive_success  =   3
}
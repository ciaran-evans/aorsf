# Generated by using Rcpp::compileAttributes() -> do not edit by hand
# Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

coxph_fit_exported <- function(x_node, y_node, w_node, method, epsilon, iter_max) {
    .Call(`_aorsf_coxph_fit_exported`, x_node, y_node, w_node, method, epsilon, iter_max)
}

linreg_fit_exported <- function(x_node, y_node, w_node, do_scale, epsilon, iter_max) {
    .Call(`_aorsf_linreg_fit_exported`, x_node, y_node, w_node, do_scale, epsilon, iter_max)
}

logreg_fit_exported <- function(x_node, y_node, w_node, do_scale, epsilon, iter_max) {
    .Call(`_aorsf_logreg_fit_exported`, x_node, y_node, w_node, do_scale, epsilon, iter_max)
}

compute_cstat_exported_vec <- function(y, w, p, pred_is_risklike) {
    .Call(`_aorsf_compute_cstat_exported_vec`, y, w, p, pred_is_risklike)
}

compute_cstat_exported_uvec <- function(y, w, g, pred_is_risklike) {
    .Call(`_aorsf_compute_cstat_exported_uvec`, y, w, g, pred_is_risklike)
}

compute_logrank_exported <- function(y, w, g) {
    .Call(`_aorsf_compute_logrank_exported`, y, w, g)
}

is_col_splittable_exported <- function(x, y, r, j) {
    .Call(`_aorsf_is_col_splittable_exported`, x, y, r, j)
}

find_cuts_survival_exported <- function(y, w, lincomb, leaf_min_events, leaf_min_obs, split_rule_R) {
    .Call(`_aorsf_find_cuts_survival_exported`, y, w, lincomb, leaf_min_events, leaf_min_obs, split_rule_R)
}

sprout_node_survival_exported <- function(y, w) {
    .Call(`_aorsf_sprout_node_survival_exported`, y, w)
}

find_rows_inbag_exported <- function(rows_oobag, n_obs) {
    .Call(`_aorsf_find_rows_inbag_exported`, rows_oobag, n_obs)
}

x_submat_mult_beta_exported <- function(x, y, w, x_rows, x_cols, beta) {
    .Call(`_aorsf_x_submat_mult_beta_exported`, x, y, w, x_rows, x_cols, beta)
}

scale_x_exported <- function(x, w) {
    .Call(`_aorsf_scale_x_exported`, x, w)
}

cph_scale <- function(x, w) {
    .Call(`_aorsf_cph_scale`, x, w)
}

orsf_cpp <- function(x, y, w, tree_type_R, tree_seeds, loaded_forest, lincomb_R_function, oobag_R_function, n_tree, mtry, sample_with_replacement, sample_fraction, vi_type_R, vi_max_pvalue, leaf_min_events, leaf_min_obs, split_rule_R, split_min_events, split_min_obs, split_min_stat, split_max_cuts, split_max_retry, lincomb_type_R, lincomb_eps, lincomb_iter_max, lincomb_scale, lincomb_alpha, lincomb_df_target, lincomb_ties_method, pred_mode, pred_type_R, pred_horizon, pred_aggregate, oobag, oobag_eval_type_R, oobag_eval_every, pd_type_R, pd_x_vals, pd_x_cols, pd_probs, n_thread, write_forest, run_forest, verbosity) {
    .Call(`_aorsf_orsf_cpp`, x, y, w, tree_type_R, tree_seeds, loaded_forest, lincomb_R_function, oobag_R_function, n_tree, mtry, sample_with_replacement, sample_fraction, vi_type_R, vi_max_pvalue, leaf_min_events, leaf_min_obs, split_rule_R, split_min_events, split_min_obs, split_min_stat, split_max_cuts, split_max_retry, lincomb_type_R, lincomb_eps, lincomb_iter_max, lincomb_scale, lincomb_alpha, lincomb_df_target, lincomb_ties_method, pred_mode, pred_type_R, pred_horizon, pred_aggregate, oobag, oobag_eval_type_R, oobag_eval_every, pd_type_R, pd_x_vals, pd_x_cols, pd_probs, n_thread, write_forest, run_forest, verbosity)
}


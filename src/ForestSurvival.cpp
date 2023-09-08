//  Forest.cpp

#include <RcppArmadillo.h>
#include "ForestSurvival.h"
#include "TreeSurvival.h"

using namespace arma;
using namespace Rcpp;

namespace aorsf {

ForestSurvival::ForestSurvival() { }

ForestSurvival::ForestSurvival(double leaf_min_events,
                               double split_min_events,
                               arma::vec& pred_horizon,
                               arma::vec& unique_event_times){

 this->leaf_min_events = leaf_min_events;
 this->split_min_events = split_min_events;
 this->pred_horizon = pred_horizon;
 this->unique_event_times = unique_event_times;

}



void ForestSurvival::load(arma::uword n_tree,
                          std::vector<std::vector<double>>& forest_cutpoint,
                          std::vector<std::vector<arma::uword>>& forest_child_left,
                          std::vector<std::vector<arma::vec>>& forest_coef_values,
                          std::vector<std::vector<arma::uvec>>& forest_coef_indices,
                          std::vector<std::vector<arma::vec>>& forest_leaf_pred_indx,
                          std::vector<std::vector<arma::vec>>& forest_leaf_pred_prob,
                          std::vector<std::vector<arma::vec>>& forest_leaf_pred_chaz,
                          std::vector<std::vector<double>>& forest_leaf_summary) {

 this->n_tree = n_tree;

 if(VERBOSITY > 0){
  Rcout << "---- loading forest from input list ----";
  Rcout << std::endl << std::endl;
 }


 // Create trees
 trees.reserve(n_tree);

 for (uword i = 0; i < n_tree; ++i) {
  trees.push_back(
   std::make_unique<TreeSurvival>(forest_cutpoint[i],
                                  forest_child_left[i],
                                  forest_coef_values[i],
                                  forest_coef_indices[i],
                                  forest_leaf_pred_indx[i],
                                  forest_leaf_pred_prob[i],
                                  forest_leaf_pred_chaz[i],
                                  forest_leaf_summary[i])
  );
 }

 // Create thread ranges
 equalSplit(thread_ranges, 0, n_tree - 1, n_thread);

}

// growInternal() in ranger
void ForestSurvival::plant() {

 trees.reserve(n_tree);

 for (arma::uword i = 0; i < n_tree; ++i) {
  trees.push_back(std::make_unique<TreeSurvival>(leaf_min_events,
                                                 split_min_events,
                                                 &unique_event_times));
 }

}

std::vector<std::vector<arma::vec>> ForestSurvival::get_leaf_pred_indx() {

 std::vector<std::vector<arma::vec>> result;

 result.reserve(n_tree);

 for (auto& tree : trees) {
  auto& temp = dynamic_cast<TreeSurvival&>(*tree);
  result.push_back(temp.get_leaf_pred_indx());
 }

 return result;

}

std::vector<std::vector<arma::vec>> ForestSurvival::get_leaf_pred_prob() {

 std::vector<std::vector<arma::vec>> result;

 result.reserve(n_tree);

 for (auto& tree : trees) {
  auto& temp = dynamic_cast<TreeSurvival&>(*tree);
  result.push_back(temp.get_leaf_pred_prob());
 }

 return result;

}

std::vector<std::vector<arma::vec>> ForestSurvival::get_leaf_pred_chaz() {

 std::vector<std::vector<arma::vec>> result;

 result.reserve(n_tree);

 for (auto& tree : trees) {
  auto& temp = dynamic_cast<TreeSurvival&>(*tree);
  result.push_back(temp.get_leaf_pred_chaz());
 }

 return result;

}


}



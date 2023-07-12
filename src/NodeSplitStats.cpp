/*-----------------------------------------------------------------------------
 This file is part of aorsf.
 Author: Byron C Jaeger
 aorsf may be modified and distributed under the terms of the MIT license.
#----------------------------------------------------------------------------*/

#include <RcppArmadillo.h>
#include "globals.h"
#include "NodeSplitStats.h"

 using namespace arma;
 using namespace Rcpp;

 namespace aorsf {

 List lrt_multi(mat& y_node,
                mat& w_node,
                vec& XB,
                uword n_split,
                double split_min_stat,
                double leaf_min_events,
                double leaf_min_obs){

  // about this function - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // this function returns a cutpoint obtaining a local maximum
  // of the log-rank test (lrt) statistic. The default value (+Inf)
  // is really for diagnostic purposes. Put another way, if the
  // return value is +Inf (an impossible value for a cutpoint),
  // that means that we didn't find any valid cut-points and
  // the node cannot be grown with the current XB.
  //
  // if there is a valid cut-point, then the main side effect
  // of this function is to modify the group vector, which
  // will be used to assign observations to the two new nodes.
  //
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  bool break_loop = false;

  vec
   group(y_node.n_rows, fill::zeros),
   vec_temp,
   cutpoints_used(n_split),
   lrt_statistics(n_split);

  double
   stat_best = 0, // initialize at the lowest possible LRT stat value
   n_events = 0,
   n_risk = 0,
   g_risk = 0,
   stat_current,
   observed,
   expected,
   V,
   temp1,
   temp2;

  uword i, j, k, list_counter = 0;

  uvec
   jit_vals,
   // sort XB- we need to iterate over the sorted indices
   XB_sorted = sort_index(XB, "ascend");

  uvec::iterator
   iit,
   jit,
   iit_best;


  // group should be initialized as all 0s
  group.zeros(y_node.n_rows);

  // initialize at the lowest possible LRT stat value
  stat_best = 0;

  // sort XB- we need to iterate over the sorted indices
  XB_sorted = sort_index(XB, "ascend");

  // unsafe columns point to cols in y_node.
  vec y_time = y_node.unsafe_col(0);
  vec y_status = y_node.unsafe_col(1);

  // first determine the lowest value of XB that will
  // be a valid cut-point to split a node. A valid cut-point
  // is one that, if used, will result in at least leaf_min_obs
  // and leaf_min_events in both the left and right node.

  n_events = 0;
  n_risk = 0;

  if(VERBOSITY > 1){
   Rcout << "----- finding cut-point boundaries -----" << std::endl;
  }

  // Iterate through the sorted values of XB, in ascending order.

  for(iit = XB_sorted.begin(); iit < XB_sorted.end()-1; ++iit){

   n_events += y_status(*iit) * w_node(*iit);
   n_risk += w_node(*iit);

   // If we want to make the current value of XB a cut-point, we need
   // to make sure the next value of XB isn't equal to this current value.
   // Otherwise, we will have the same value of XB in both groups!

   if(VERBOSITY > 1){
    Rcout << XB(*iit)     << " ---- ";
    Rcout << XB(*(iit+1)) << " ---- ";
    Rcout << n_events     << " ---- ";
    Rcout << n_risk       << std::endl;
   }

   if(XB(*iit) != XB(*(iit+1))){

    if(VERBOSITY > 1){
     Rcout << "********* New cut-point here ********" << std::endl;
    }


    if( n_events >= leaf_min_events &&
        n_risk   >= leaf_min_obs) {

     if(VERBOSITY > 1){
      Rcout << std::endl;
      Rcout << "lower cutpoint: "         << XB(*iit) << std::endl;
      Rcout << " - n_events, left node: " << n_events << std::endl;
      Rcout << " - n_risk, left node:   " << n_risk   << std::endl;
      Rcout << std::endl;
     }

     break;

    }

   }

  }

  if(VERBOSITY > 1){
   if(iit >= XB_sorted.end()-1) {
    Rcout << "Could not find a valid lower cut-point" << std::endl;
   }
  }

  j = iit - XB_sorted.begin();

  // got to reset these before finding the upper limit
  n_events=0;
  n_risk=0;

  // do the first step in the loop manually since we need to
  // refer to iit+1 in all proceeding steps.

  for(iit = XB_sorted.end()-1; iit >= XB_sorted.begin()+1; --iit){

   n_events += y_status(*iit) * w_node(*iit);
   n_risk   += w_node(*iit);
   group(*iit) = 1;

   if(VERBOSITY > 1){
    Rcout << XB(*iit)     << " ---- ";
    Rcout << XB(*(iit-1)) << " ---- ";
    Rcout << n_events     << " ---- ";
    Rcout << n_risk       << std::endl;
   }

   if(XB(*iit) != XB(*(iit-1))){

    if(VERBOSITY > 1){
     Rcout << "********* New cut-point here ********" << std::endl;
    }

    if( n_events >= leaf_min_events &&
        n_risk   >= leaf_min_obs ) {

     // the upper cutpoint needs to be one step below the current
     // iit value, because we use x <= cp to determine whether a
     // value x goes to the left node versus the right node. So,
     // if iit currently points to 3, and the next value down is 2,
     // then we want to say the cut-point is 2 because then all
     // values <= 2 will go left, and 3 will go right. This matters
     // when 3 is the highest value in the vector.

     --iit;

     if(VERBOSITY > 1){
      Rcout << std::endl;
      Rcout << "upper cutpoint: " << XB(*iit) << std::endl;
      Rcout << " - n_events, right node: " << n_events    << std::endl;
      Rcout << " - n_risk, right node:   " << n_risk      << std::endl;
     }

     break;

    }

   }

  }

  // number of steps taken
  k = iit + 1 - XB_sorted.begin();

  if(VERBOSITY > 1){
   Rcout << "----------------------------------------" << std::endl;
   Rcout << std::endl << std::endl;
   Rcout << "sorted XB: " << std::endl << XB(XB_sorted).t() << std::endl;
  }

  // initialize cut-point as the value of XB iit currently points to.
  iit_best = iit;

  // what happens if we don't have enough events or obs to split?
  // the first valid lower cut-point (at XB_sorted(k)) is > the first
  // valid upper cutpoint (current value of n_risk). Put another way,
  // k (the number of steps taken from beginning of the XB vec)
  // will be > n_rows - p, where the difference on the RHS is
  // telling us where we are after taking p steps from the end
  // of the XB vec. Returning the infinite cp is a red flag.

  if(VERBOSITY > 1){
   Rcout << "j: " << j << std::endl;
   Rcout << "k: " << k << std::endl;
  }

  if (j > k){

   if(VERBOSITY > 1) {
    Rcout << "Could not find a cut-point for this XB" << std::endl;
   }

   return(List::create(_["cutpoints"] = R_PosInf,
                       _["statistic"] = R_PosInf));

  }

  if(VERBOSITY > 1){

   Rcout << "----- initializing log-rank test cutpoints -----" << std::endl;
   Rcout << "n potential cutpoints: " << k-j << std::endl;

  }

  // what happens if there are only 5 potential cut-points
  // but the value of n_split is > 5? We will just check out
  // the 5 valid cutpoints.

  // adjust k to indicate steps taken in the outer loop.
  k -= j;

  if(k > n_split){

   jit_vals = linspace<uvec>(0, k, n_split);

  } else {

   jit_vals = linspace<uvec>(0, k, k);

  }

  vec_temp.resize( jit_vals.size() );

  if(j == 0) jit_vals(jit_vals.size()-1)--;

  for(k = 0; k < vec_temp.size(); k++){
   vec_temp(k) = XB(*(iit_best - jit_vals(k)));
  }

  if(j == 0) jit_vals(jit_vals.size()-1)++;


  if(VERBOSITY > 1){

   Rcout << "cut-points chosen: ";

   Rcout << vec_temp.t();

   Rcout << "----------------------------------------" << std::endl <<
    std::endl << std::endl;

  }

  bool do_lrt = true;

  k = 0;
  j = 1;

  // begin outer loop - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  for(jit = jit_vals.begin(); jit != jit_vals.end(); ++jit){

   for( ; j < *jit; j++){
    group(*iit) = 1;
    --iit;
   }

   if(jit == jit_vals.begin() ||
      jit == jit_vals.end()-1){

    do_lrt = true;

   } else {

    if( vec_temp(k) == vec_temp(k+1) ||
        vec_temp(k) == vec_temp(0)   ||
        *jit <= 1){

        do_lrt = false;

    } else {

     while(XB(*iit) == XB(*(iit - 1))){

      group(*iit) = 1;
      --iit;
      ++j;

      if(VERBOSITY > 1){
       Rcout << "cutpoint dropped down one spot: ";
       Rcout << XB(*iit) << std::endl;
      }

     }

     do_lrt = true;

    }

   }

   ++k;

   if(do_lrt){

    cutpoints_used(list_counter) = XB(*iit);

    n_risk=0;
    g_risk=0;

    observed=0;
    expected=0;

    V=0;

    break_loop = false;

    i = y_node.n_rows-1;

    if(VERBOSITY > 1){
     Rcout << "sum(group==1): " << sum(group) << ";  ";
     Rcout << "sum(group==1 * w_node): " << sum(group % w_node);
     Rcout << std::endl;
     if(VERBOSITY > 1){
      Rcout << "group:" << std::endl;
      Rcout << group(XB_sorted).t() << std::endl;
     }
    }


    // begin inner loop  - - - - - - - - - - - - -  - - - - - - - - - - - - -
    for (; ;){

     temp1 = y_time[i];

     n_events = 0;

     for ( ; y_time[i] == temp1; i--) {

      n_risk += w_node[i];
      n_events += y_status[i] * w_node[i];
      g_risk += group[i] * w_node[i];
      observed += y_status[i] * group[i] * w_node[i];

      if(i == 0){
       break_loop = true;
       break;
      }

     }

     // should only do these calculations if n_events > 0,
     // but turns out its faster to multiply by 0 than
     // it is to check whether n_events is > 0

     temp2 = g_risk / n_risk;
     expected += n_events * temp2;

     // update variance if n_risk > 1 (if n_risk == 1, variance is 0)
     // definitely check if n_risk is > 1 b/c otherwise divide by 0
     if (n_risk > 1){
      temp1 = n_events * temp2 * (n_risk-n_events) / (n_risk-1);
      V += temp1 * (1 - temp2);
     }

     if(break_loop) break;

    }
    // end inner loop  - - - - - - - - - - - - -  - - - - - - - - - - - - - - -

    stat_current = pow(expected-observed, 2) / V;

    lrt_statistics(list_counter) = stat_current;

    list_counter++;

    if(VERBOSITY > 1){

     Rcout << "-------- log-rank test results --------" << std::endl;
     Rcout << "cutpoint: " << XB(*iit)                  << std::endl;
     Rcout << "lrt stat: " << stat_current              << std::endl;
     Rcout << "---------------------------------------" << std::endl <<
      std::endl << std::endl;

    }

    if(stat_current > stat_best){
     iit_best = iit;
     stat_best = stat_current;
    }

   }
   // end outer loop  - - - - - - - - - - - - -  - - - - - - - - - - - - - - -

  }

  // if the log-rank test does not detect a difference at 0.05 alpha,
  // maybe it's not a good idea to split this node.

  // if(stat_best < 3.841459) return(
  //   List::create(_["cutpoints"] = R_PosInf,
  //                _["statistic"] = R_PosInf)
  // );

  if(VERBOSITY > 1){
   Rcout << "Best LRT stat: " << stat_best << std::endl;
  }

  // rewind iit until it is back where it was when we got the
  // best lrt stat. While rewinding iit, also reset the group
  // values so that group is as it was when we got the best
  // lrt stat.


  while(iit <= iit_best){
   group(*iit) = 0;
   ++iit;
  }

  return(List::create(_["cutpoints"] = cutpoints_used,
                      _["statistic"] = lrt_statistics,
                      _["vec_temp"] = vec_temp));

 }

 // List lrt_multi(mat& y_node,
 //                mat& w_node,
 //                vec& XB,
 //                uword n_split,
 //                double split_min_stat,
 //                double leaf_min_obs,
 //                double leaf_min_events){
 //
 //  bool break_loop = false;
 //
 //  vec
 //   group(y_node.n_rows, fill::zeros),
 //   potential_cutpoints,
 //   cutpoints_used(n_split),
 //   lrt_statistics(n_split);
 //
 //  double
 //   stat_best = 0, // initialize at the lowest possible LRT stat value
 //   n_events = 0,
 //   n_risk = 0,
 //   g_risk = 0,
 //   stat_current,
 //   observed,
 //   expected,
 //   variance,
 //   temp1,
 //   temp2;
 //
 //  uword i, j, k, list_counter = 0;
 //
 //  uvec
 //   jit_vals,
 //   // sort XB- we need to iterate over the sorted indices
 //   XB_sorted = sort_index(XB, "ascend");
 //
 //  uvec::iterator
 //   iit,
 //   jit,
 //   iit_best;
 //
 //  // unsafe columns point to cols in y_node.
 //  vec y_status = y_node.unsafe_col(1);
 //  vec y_time = y_node.unsafe_col(0);
 //
 //  // first determine the lowest value of XB that will
 //  // be a valid cut-point to split a node. A valid cut-point
 //  // is one that, if used, will result in at least leaf_min_obs
 //  // and leaf_min_events in both the left and right node.
 //
 //
 //  if(VERBOSITY > 0){
 //   Rcout << "----- finding cut-point boundaries -----" << std::endl;
 //  }
 //
 //  // Iterate through the sorted values of XB, in ascending order.
 //
 //  for(iit = XB_sorted.begin(); iit < XB_sorted.end()-1; ++iit){
 //
 //   n_events += y_status[*iit] * w_node[*iit];
 //   n_risk += w_node[*iit];
 //
 //   // If we want to make the current value of XB a cut-point, we need
 //   // to make sure the next value of XB isn't equal to this current value.
 //   // Otherwise, we will have the same value of XB in both groups!
 //
 //   if(VERBOSITY > 0){
 //    Rcout << XB[*iit]     << " ---- ";
 //    Rcout << XB[*(iit+1)] << " ---- ";
 //    Rcout << n_events     << " ---- ";
 //    Rcout << n_risk       << std::endl;
 //   }
 //
 //   if(XB[*iit] != XB[*(iit+1)]){
 //
 //    if(VERBOSITY > 0){
 //     Rcout << "********* New cut-point here ********" << std::endl;
 //    }
 //
 //
 //    if( n_events >= leaf_min_events &&
 //        n_risk   >= leaf_min_obs) {
 //
 //     if(VERBOSITY > 0){
 //      Rcout << std::endl;
 //      Rcout << "lower cutpoint: "         << XB[*iit] << std::endl;
 //      Rcout << " - n_events, left node: " << n_events << std::endl;
 //      Rcout << " - n_risk, left node:   " << n_risk   << std::endl;
 //      Rcout << std::endl;
 //     }
 //
 //     break;
 //
 //    }
 //
 //   }
 //
 //  }
 //
 //  if(VERBOSITY > 0){
 //   if(iit >= XB_sorted.end()-1) {
 //    Rcout << "Could not find a valid lower cut-point" << std::endl;
 //   }
 //  }
 //
 //
 //  j = iit - XB_sorted.begin();
 //
 //  // got to reset these before finding the upper limit
 //  n_events=0;
 //  n_risk=0;
 //
 //  // do the first step in the loop manually since we need to
 //  // refer to iit+1 in all proceeding steps.
 //
 //  for(iit = XB_sorted.end()-1; iit >= XB_sorted.begin()+1; --iit){
 //
 //   n_events += y_status[*iit] * w_node[*iit];
 //   n_risk   += w_node[*iit];
 //   group[*iit] = 1;
 //
 //   if(VERBOSITY > 0){
 //    Rcout << XB[*iit]     << " ---- ";
 //    Rcout << XB(*(iit-1)) << " ---- ";
 //    Rcout << n_events     << " ---- ";
 //    Rcout << n_risk       << std::endl;
 //   }
 //
 //   if ( XB[*iit] != XB[*(iit-1)] ) {
 //
 //    if(VERBOSITY > 0){
 //     Rcout << "********* New cut-point here ********" << std::endl;
 //    }
 //
 //    if( n_events >= leaf_min_events &&
 //        n_risk   >= leaf_min_obs ) {
 //
 //     // the upper cutpoint needs to be one step below the current
 //     // iit value, because we use x <= cp to determine whether a
 //     // value x goes to the left node versus the right node. So,
 //     // if iit currently points to 3, and the next value down is 2,
 //     // then we want to say the cut-point is 2 because then all
 //     // values <= 2 will go left, and 3 will go right. This matters
 //     // when 3 is the highest value in the vector.
 //
 //     --iit;
 //
 //     if(VERBOSITY > 0){
 //      Rcout << std::endl;
 //      Rcout << "upper cutpoint: " << XB[*iit] << std::endl;
 //      Rcout << " - n_events, right node: " << n_events    << std::endl;
 //      Rcout << " - n_risk, right node:   " << n_risk      << std::endl;
 //     }
 //
 //     break;
 //
 //    }
 //
 //   }
 //
 //  }
 //
 //  // number of steps taken
 //  k = iit + 1 - XB_sorted.begin();
 //
 //  if(VERBOSITY > 0){
 //   Rcout << "----------------------------------------" << std::endl;
 //   Rcout << std::endl << std::endl;
 //   Rcout << "sorted XB: " << std::endl << XB(XB_sorted).t() << std::endl;
 //  }
 //
 //  // initialize cut-point as the value of XB iit currently points to.
 //  iit_best = iit;
 //
 //  // what happens if we don't have enough events or obs to split?
 //  // the first valid lower cut-point (at XB_sorted(k)) is > the first
 //  // valid upper cutpoint (current value of n_risk). Put another way,
 //  // k (the number of steps taken from beginning of the XB vec)
 //  // will be > n_rows - p, where the difference on the RHS is
 //  // telling us where we are after taking p steps from the end
 //  // of the XB vec. Returning the infinite cp is a red flag.
 //
 //  if(VERBOSITY > 0){
 //   Rcout << "j: " << j << std::endl;
 //   Rcout << "k: " << k << std::endl;
 //  }
 //
 //  if (j > k){
 //
 //   if(VERBOSITY > 0) {
 //    Rcout << "Could not find a cut-point for this XB" << std::endl;
 //   }
 //
 //   return(R_PosInf);
 //  }
 //
 //  if(VERBOSITY > 0){
 //
 //   Rcout << "----- initializing log-rank test cutpoints -----" << std::endl;
 //   Rcout << "n potential cutpoints: " << k-j << std::endl;
 //
 //  }
 //
 //
 //  // adjust k to indicate the number of valid cut-points
 //  k -= j;
 //
 //  if(k > n_split){
 //
 //   jit_vals = linspace<uvec>(0, k, n_split);
 //
 //  } else {
 //
 //   // what happens if there are only 5 potential cut-points
 //   // but the value of n_split is > 5? We will just check out
 //   // the 5 valid cutpoints.
 //   jit_vals = linspace<uvec>(0, k, k);
 //
 //  }
 //
 //  potential_cutpoints.resize( jit_vals.size() );
 //
 //  // protection from going out of bounds with jit_vals(k) below
 //  if(j == 0) jit_vals(jit_vals.size()-1)--;
 //
 //  // put the indices of potential cut-points into potential_cutpoints
 //  for(k = 0; k < potential_cutpoints.size(); k++){
 //   potential_cutpoints[k] = XB(*(iit_best - jit_vals[k]));
 //  }
 //
 //  // back to how it was!
 //  if(j == 0) jit_vals(jit_vals.size()-1)++;
 //
 //  if(VERBOSITY > 0){
 //
 //   Rcout << "cut-points chosen: ";
 //
 //   Rcout << potential_cutpoints.t();
 //
 //   Rcout << "----------------------------------------" << std::endl <<
 //    std::endl << std::endl;
 //
 //  }
 //
 //  bool do_lrt = true;
 //
 //  k = 0;
 //  j = 1;
 //
 //  // begin outer loop - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //  for(jit = jit_vals.begin(); jit != jit_vals.end(); ++jit){
 //
 //
 //   if(VERBOSITY > 1){
 //    Rcout << "jit points to " << *jit << std::endl;
 //   }
 //
 //   // switch group values from 0 to 1 until you get to the next cut-point
 //   for( ; j < *jit; j++){
 //    group[*iit] = 1;
 //    --iit;
 //   }
 //
 //   if(jit == jit_vals.begin() ||
 //      jit == jit_vals.end()-1){
 //
 //    do_lrt = true;
 //
 //   } else {
 //
 //    if( potential_cutpoints[k] == potential_cutpoints[k+1] ||
 //        potential_cutpoints[k] == potential_cutpoints[0]   ||
 //        *jit <= 1){
 //
 //        do_lrt = false;
 //
 //    } else {
 //
 //     while( XB[*iit] == XB[*(iit - 1)] ){
 //
 //      group[*iit] = 1;
 //      --iit;
 //      ++j;
 //
 //      if(VERBOSITY > 1){
 //       Rcout << "cutpoint dropped down one spot: ";
 //       Rcout << XB[*iit] << std::endl;
 //      }
 //
 //     }
 //
 //     do_lrt = true;
 //
 //    }
 //
 //   }
 //
 //   ++k;
 //
 //   if(do_lrt){
 //
 //    cutpoints_used(list_counter) = XB(*iit);
 //
 //    n_risk=0;
 //    g_risk=0;
 //
 //    observed=0;
 //    expected=0;
 //    variance=0;
 //
 //    break_loop = false;
 //
 //    i = y_node.n_rows-1;
 //
 //    if(VERBOSITY > 0){
 //     Rcout << "sum(group==1): " << sum(group) << ";  ";
 //     Rcout << "sum(group==1 * w_node): " << sum(group % w_node);
 //     Rcout << std::endl;
 //     if(VERBOSITY > 0){
 //      Rcout << "group:" << std::endl;
 //      Rcout << group(XB_sorted).t() << std::endl;
 //     }
 //    }
 //
 //
 //    // begin inner loop  - - - - - - - - - - - - -  - - - - - - - - - - - - -
 //    for (; ;){
 //
 //     temp1 = y_time[i];
 //
 //     n_events = 0;
 //
 //     for ( ; y_time[i] == temp1; i--) {
 //
 //      n_risk += w_node[i];
 //      n_events += y_status[i] * w_node[i];
 //      g_risk += group[i] * w_node[i];
 //      observed += y_status[i] * group[i] * w_node[i];
 //
 //      if(i == 0){
 //       break_loop = true;
 //       break;
 //      }
 //
 //     }
 //
 //     // should only do these calculations if n_events > 0,
 //     // but turns out its faster to multiply by 0 than
 //     // it is to check whether n_events is > 0
 //
 //     temp2 = g_risk / n_risk;
 //     expected += n_events * temp2;
 //
 //     // update variance if n_risk > 1 (if n_risk == 1, variance is 0)
 //     // definitely check if n_risk is > 1 b/c otherwise divide by 0
 //     if (n_risk > 1){
 //      temp1 = n_events * temp2 * (n_risk-n_events) / (n_risk-1);
 //      variance += temp1 * (1 - temp2);
 //     }
 //
 //     if(break_loop) break;
 //
 //    }
 //    // end inner loop  - - - - - - - - - - - - -  - - - - - - - - - - - - - - -
 //
 //    stat_current = pow(expected-observed, 2) / variance;
 //
 //    lrt_statistics(list_counter) = stat_current;
 //    list_counter++;
 //
 //    if(VERBOSITY > 0){
 //
 //     Rcout << "-------- log-rank test results --------" << std::endl;
 //     Rcout << "cutpoint: " << XB[*iit]                  << std::endl;
 //     Rcout << "lrt stat: " << stat_current              << std::endl;
 //     Rcout << "---------------------------------------" << std::endl <<
 //      std::endl << std::endl;
 //
 //    }
 //
 //    if(stat_current > stat_best){
 //     iit_best = iit;
 //     stat_best = stat_current;
 //    }
 //
 //   }
 //   // end outer loop  - - - - - - - - - - - - -  - - - - - - - - - - - - - - -
 //
 //  }
 //
 //  // if the log-rank test does not detect a difference at 0.05 alpha,
 //  // maybe it's not a good idea to split this node.
 //
 //  if(stat_best < split_min_stat) return(R_PosInf);
 //
 //  if(VERBOSITY > 0){
 //   Rcout << "Best LRT stat: " << stat_best << std::endl;
 //  }
 //
 //  // rewind iit until it is back where it was when we got the
 //  // best lrt stat. While rewinding iit, also reset the group
 //  // values so that group is as it was when we got the best
 //  // lrt stat.
 //
 //  while(iit <= iit_best){
 //   group[*iit] = 0;
 //   ++iit;
 //  }
 //
 //  // XB at *iit_best is the cut-point that maximized the log-rank test
 //  return(List::create(_["cutpoints"] = cutpoints_used,
 //                      _["statistic"] = lrt_statistics,
 //                      _["best_cut"]  = XB[*iit_best]));
 //
 // }


 }


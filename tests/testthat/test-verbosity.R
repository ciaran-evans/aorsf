

test_that(
 desc = 'verbosity prints grow, predict, and importance notes',
 code = {

  expect_snapshot(
   fit_verbose <- orsf(pbc, time + status ~.,
                       verbose_progress = TRUE,
                       n_tree = n_tree_test,
                       importance = 'negate')
  )

  expect_snapshot(
   fit_verbose <- orsf(pbc, time + status ~.,
                       verbose_progress = TRUE,
                       n_tree = n_tree_test,
                       importance = 'negate',
                       n_thread = 5)
  )

 }
)




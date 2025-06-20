# Generated by using Rcpp::compileAttributes() -> do not edit by hand
# Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

cpp_get_max_thread <- function() {
    .Call('_wordvector_cpp_get_max_thread', PACKAGE = 'wordvector')
}

cpp_w2v <- function(xptr, size = 100L, window = 5L, sample = 0.001, withHS = FALSE, negative = 5L, threads = 1L, iterations = 5L, alpha = 0.05, type = 1L, verbose = FALSE, normalize = TRUE, model = NULL) {
    .Call('_wordvector_cpp_w2v', PACKAGE = 'wordvector', xptr, size, window, sample, withHS, negative, threads, iterations, alpha, type, verbose, normalize, model)
}


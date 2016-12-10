// [[Rcpp::depends(BH, bigmemory)]]
#include <Rcpp.h>
#include <bigmemory/MatrixAccessor.hpp>

using namespace Rcpp;


/******************************************************************************/

// [[Rcpp::export]]
LogicalVector clumping(SEXP pBigMat,
                       const IntegerVector& rowInd,
                       const IntegerVector& colInd,
                       LogicalVector& remain,
                       const NumericVector& sumX,
                       const NumericVector& denoX,
                       int size,
                       double thr) {

  XPtr<BigMatrix> xpMat(pBigMat);
  MatrixAccessor<char> macc(*xpMat);

  int n = rowInd.size();
  int m = colInd.size();
  double nd = (double)n;

  // indices begin at 1 in R and 0 in C++
  IntegerVector trains = rowInd - 1;
  double xySum, num, r2;
  int i, ind_i, j, j0, k;

  LogicalVector keep(m);

  for (k = 0; k < m; k++) {
    j0 = colInd[k] - 1;
    if (remain[j0]) { // if already excluded, goto next
      for (j = max(0, j0 - size); j < min(m, j0 + size); j++) {
        if (remain[j]) { // if already excluded, goto next
          xySum = 0;
          for (i = 0; i < n; i++) {
            ind_i = trains[i];
            xySum += macc[j][ind_i] * macc[j0][ind_i];
          }
          num = xySum - sumX[j] * sumX[j0] / nd;
          r2 = num * num / (denoX[j] * denoX[j0]);
          if (r2 > thr) remain[j] = false; // prune
        }
      }
      keep[j0] = true;
      remain[j0] = false;
    }
  }

  return keep;
}

/******************************************************************************/

// [[Rcpp::export]]
LogicalVector& pruning(SEXP pBigMat,
                       const IntegerVector& rowInd,
                       LogicalVector& keep,
                       const NumericVector& mafX,
                       const NumericVector& sumX,
                       const NumericVector& denoX,
                       int size,
                       double thr) {
  // Assert that keep[j] == TRUE
  XPtr<BigMatrix> xpMat(pBigMat);
  MatrixAccessor<char> macc(*xpMat);

  // indices begin at 1 in R and 0 in C++
  IntegerVector trains = rowInd - 1;

  int n = rowInd.size();
  double nd = (double)n;
  int m = xpMat->ncol();
  double xySum, num, r2;

  int j0, j, i, ind_i;

  for (j0 = 1; j0 < size; j0++) {
    if (keep[j0]) { // if already excluded, goto next
      for (j = 0; j < j0; j++) {
        if (keep[j]) { // if already excluded, goto next
          xySum = 0;
          for (i = 0; i < n; i++) {
            ind_i = trains[i];
            xySum += macc[j][ind_i] * macc[j0][ind_i];
          }
          num = xySum - sumX[j] * sumX[j0] / nd;
          r2 = num * num / (denoX[j] * denoX[j0]);
          if (r2 > thr) { // prune one of them
            if (mafX[j0] < mafX[j]) { // prune the one with smaller maf
              keep[j0] = false;
              break;
            } else {
              keep[j] = false;
            }
          }
        }
      }
    }
  }

  for (j0 = size; j0 < m; j0++) {
    if (keep[j0]) { // if already excluded, goto next
      for (j = j0 - size + 1; j < j0; j++) {
        if (keep[j]) { // if already excluded, goto next
          xySum = 0;
          for (i = 0; i < n; i++) {
            ind_i = trains[i];
            xySum += macc[j][ind_i] * macc[j0][ind_i];
          }
          num = xySum - sumX[j] * sumX[j0] / nd;
          r2 = num * num / (denoX[j] * denoX[j0]);
          if (r2 > thr) { // prune one of them
            if (mafX[j] < mafX[j0]) { // prune the one with smaller maf
              keep[j] = false;
            } else {
              keep[j0] = false;
              break;
            }
          }
        }
      }
    }
  }

  return(keep);
}

/******************************************************************************/

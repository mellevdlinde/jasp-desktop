# functions to replace JASP's rcpp functions

.readDatasetToEndNative <- function(columns = c(), columns.as.numeric = c(), columns.as.ordinal = c(),
                                    columns.as.factor = c(), all.columns = FALSE) {

  dataset <- .getInternal("dataset")
  dataset <- .loadCorrectDataset(dataset)

  envir <- .getInternal("envir")
  if (all.columns) {
    columns <- colnames(dataset)
    columns <- columns[columns != ""]
  }
  dataset <- envir$.vdf(dataset, columns, columns.as.numeric, columns.as.ordinal,
                        columns.as.factor, all.columns, exclude.na.listwise = c())

  return(dataset)
}

.readDataSetHeaderNative <- function(columns = c(), columns.as.numeric = c(), columns.as.ordinal = c(),
                                     columns.as.factor = c(), all.columns = FALSE) {

  dataset <- .readDatasetToEndNative(columns, columns.as.numeric, columns.as.ordinal,
                                     columns.as.factor, all.columns)
  dataset <- dataset[0, , drop = FALSE]

  return(dataset)
}

.requestTempFileNameNative <- function(...) {
  root <- file.path(tempdir(), "jasptools", "html")
  numPlots <- length(list.files(file.path(root, "plots")))
  list(
    root = root,
    relativePath = file.path("plots", paste0(numPlots + 1, ".png"))
  )
}

.requestStateFileNameNative <- function() {
  root <- file.path(tempdir(), "jasptools", "state")
  name <- "state"
  list(
    root = root,
    relativePath = name
  )
}

.callbackNative <- function(...) {
  list(status="ok")
}

.imageBackground <- function(...) return("white")
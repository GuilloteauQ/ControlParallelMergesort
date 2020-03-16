# Parameters
# - size of the array
# - initial block size -> n / 2p
# - reference -> p
# - kp
# - percentage
# - nb of threads ?

options(scipen=999)

number_of_runs <- 15
p <- 4

power10 <- function(x) {
    10 ^ x
}
# Array size
min_array_size_log <- 3
max_array_size_log <- 8
array_sizes <- power10(seq(min_array_size_log, max_array_size_log, 1))
# KP
max_kp = 10
inc_kp = 0.5
kp <- seq(1, max_kp, inc_kp)
# Percentage
percentage <- seq(0, 100, 10)

data_frame <- expand.grid(array_sizes, kp, percentage)
colnames(data_frame) <- c("array_size", "kp", "percentage")

#data_frame <- lhs.design(
#                         type = "maximin",
#                         nruns = number_of_runs,
#                         nfactors = 3,
#                         digits = 0,
#                         seed = 17440,
#                         factor.names = list(array_size = c(min_array_size, max_array_size),
#                                             kp = c(1, max_kp),
#                                             percentage = c(0, 100)))
#
data_frame$block_size <- data_frame$array_size / (2 * p)
data_frame$reference <- p

final_df <- data_frame
for (i in seq(1, number_of_runs)) {
    final_df <- rbind(data_frame, final_df)
    # rownames(DFtranspose) <- DF1[1, ]
}

# Shuffle the data frame
final_df <- final_df[sample(nrow(final_df)),]

# data_frame
write.csv(final_df, "doe.csv")

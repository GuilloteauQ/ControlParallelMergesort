setwd("../..")

csvfile <- "analysis/experiments/doe.csv"

df <- read.csv(file = csvfile)

head(df)

system("make > /dev/null")

commands <- paste("./feedback_sort",
                  df$array_size,
                  df$block_size,
                  df$reference,
                  df$kp,
                  df$percentage,
                  sep = " ")
for (command in commands) {
    system(command)
}


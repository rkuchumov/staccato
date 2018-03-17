#!/usr/bin/env Rscript

library('ggplot2')

read_input <- function (path) {
    data.raw <- read.table(input, header=TRUE)

    sdev <- function (x) sqrt(var(x))
    data.mean <- aggregate(time ~ sched + name + threads, data.raw, mean)
    data.sdev <- aggregate(time ~ sched + name + threads, data.raw, sdev)
    data <- cbind(data.mean, sdev = data.sdev$time)
    return(data)
}

my_plot <- function (data, name, title = 'atata') {
    d <- data[data$name==name,]

    # t <- log(time)
    p <- ggplot(d, aes(x = threads, y = log(time), group = sched, color = sched)) +
        geom_line() +
        geom_point() +
        # geom_errorbar(
        #     aes(ymin = t - sdev, ymax = t + sdev),
        #     width = .2,
        #     position = position_dodge(0.05)
        # ) +
        labs(
            title = title,
            x = 'Number of threads',
            y = 'Execution time (us), log scale'
        ) +
        scale_color_manual(values=c('#999999','#E69F00', '#4286f4'))

    return(p)
}

process <- function (data, benchmark, title, output) {
    p <- my_plot(data, benchmark, title)

    path <- paste(output, '-', benchmark, '.png', sep='')
    ggsave(path, p, width = 10, height = 6)
}


args = commandArgs(trailingOnly=TRUE)
if (length(args) == 0) {
    stop("Usage: ./plot.r <data_file>", call.=FALSE)
}

input <- args[1]

data <- read_input(input)

process(data, 'fib', 'Fibonacci Number (43)', input)
process(data, 'dfs', 'Depth First Search (9^10 vertices)', input)
process(data, 'matmul', 'Matrix Multiplication (3500x3500)', input)
process(data, 'mergesort', 'Merge Sort (10^9 of 4 byte integers)', input)
process(data, 'blkmul', 'Block Matrix Multiplication (4096*4096)', input)



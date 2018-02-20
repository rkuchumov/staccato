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
    d <- data[data$name=='fib',]

    p <- ggplot(d, aes(x = threads, y = time, group = sched, color = sched)) +
        geom_line() +
        geom_point() +
        geom_errorbar(
            aes(ymin = time - sdev, ymax = time + sdev),
            width = .2,
            position = position_dodge(0.05)
        ) +
        labs(
            title = title,
            x = 'Number of threads',
            y = 'Execution time (us)'
        ) +
        scale_color_manual(values=c('#999999','#E69F00'))

    return(p)
}

save_plot <- function(p, path) {
    png(path, width=800, height=300, pointsize=50)
    print(p)
    dev.off()
}

args = commandArgs(trailingOnly=TRUE)
if (length(args) == 0) {
    stop("Usage: ./plot.r <data_file> <benchmark>", call.=FALSE)
}

input <- args[1]
benchmark <- args[2]
output <- paste(input, '-', benchmark, '.png', sep='')

data <- read_input(input)
p <- my_plot(data, benchmark)
save_plot(p, output)


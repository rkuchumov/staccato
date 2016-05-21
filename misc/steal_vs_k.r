data2 <- read.table('out_2.tab', header = T, dec = '.')
data4 <- read.table('out_4.tab', header = T, dec = '.')
data8 <- read.table('out_8.tab', header = T, dec = '.')
data16 <- read.table('out_16.tab', header = T, dec = '.')
data24 <- read.table('out_24.tab', header = T, dec = '.')
data32 <- read.table('out_32.tab', header = T, dec = '.')

lambdas <- unique(data2$lambda)
colors <- rainbow(length(lambdas))


draw.data <- function(data, title) {
	for (l in lambdas) {
		d <- data[data$l == l, ]
		# data[data$l == l, ]$delay <- d$delay / d$delay[1]
		data[data$l == l, ]$steals <- d$steals / d$steals[1]
	}

	plot(0, 0,
		 # ylim = c(0.95, max(data$delay)), xlim = c(0, 20),
		 ylim = c(0.7, max(data$steals)), xlim = c(0, 20),
		 type ='l', lwd = 2.5,
		 xlab = 'Размер кражи (k)',
		 # ylab = 'Изменение задержки',
		 ylab = 'Изменение количества краж',
		 main = title
	)



	i <- 1
	for (l in lambdas) {
		d <- data[data$l == l, ]
		# lines(x = d$k, y = d$delay, col = colors[i])
		lines(x = d$k, y = d$steals, col = colors[i])

		i <- i + 1

	}

	grid()
}


# png("delay_vs_k.png", width = 1000, height = 800, pointsize = 18)
png("steals_vs_k.png", width = 1000, height = 800, pointsize = 18)

layout(matrix(c(1,2,3,4,5,6,7,7,7), ncol = 3, byrow=TRUE), heights=c(4, 4, 1))
par(mai=rep(0.75, 4))

draw.data(data2, '2 Процессора')
draw.data(data4, '4 Процессора')
draw.data(data8, '8 Процессоров')
draw.data(data16, '16 Процессоров')
draw.data(data24, '24 Процессора')
draw.data(data32, '32 Процессора')

par(mai=c(0,0,0,0))
plot.new()


legend("center", ncol = length(lambdas), legend = paste0('λ = ', lambdas), col=colors, lwd = 2.5)
dev.off()

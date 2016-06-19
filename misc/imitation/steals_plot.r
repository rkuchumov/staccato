draw.data <- function(data, title, x = F, y = F) {
	for (l in lambdas) {
		d <- data[data$rate == l, ]
		data[data$rate == l, ]$steals <- d$steals / d$steals[1]
	}

	plot(0, 0,
		 ylim = c(0.7, max(data$steals)), xlim = c(0, 20),
		 type ='l', lwd = 2.5,
		 xlab = '', ylab = '', main = title,
		 axes = F,
	)
	
	axis(side = 1, labels = x)
	axis(side = 2, labels = y)

	i <- 1
	for (l in lambdas) {
		d <- data[data$rate == l, ]
		lines(x = d$k, y = d$steals, lwd = 1.5, lty = lty[i] )
		points(x = d$k, y = d$steals, pch = pch[i])

		i <- i + 1
	}

	grid()
	box()
}

data2 <-  read.table('imi2.txt',  header = T, sep=';', dec = '.')
data4 <-  read.table('imi4.txt',  header = T, sep=';', dec = '.')
data8 <-  read.table('imi8.txt',  header = T, sep=';', dec = '.')
data16 <- read.table('imi16.txt', header = T, sep=';', dec = '.')
data24 <- read.table('imi24.txt', header = T, sep=';', dec = '.')
data32 <- read.table('imi32.txt', header = T, sep=';', dec = '.')

lambdas <- unique(data2$rate)
colors <- rainbow(length(lambdas))
pch <- c(0, 3, 4, 5, 1, 2)
lty <- c(6, 3, 5, 2, 4, 1)

# setEPS()
# postscript("imitation.eps", pointsize=17, width=8, height=11)
# png("imitation.png", width = 1000, height = 800, pointsize = 18)

layout(matrix(c(1,2,3,4,5,6,7,7), ncol = 2, byrow=TRUE), heights=c(4, 4, 4, 1))

par(
	oma = c(2.5, 3, 0, 0),
	mar = c(1, 1, 1.3, 0),
	mgp = c(2, 1, 0)
	)

draw.data(data2, '2 Processors', F, T)
draw.data(data4, '4 Processors')
draw.data(data8, '8 Processors', F, T)
draw.data(data16, '16 Processors')
draw.data(data24, '24 Processors', T, T)
draw.data(data32, '32 Processors', T, F)

title(xlab = "Steal Size (k)", outer = TRUE, line = -2.3, cex.lab = 1.3)
title(ylab = 'Deviation of # Steals', outer = TRUE, line = 1.5, cex.lab = 1.3)

par(
	oma = c(0, 0, 0, 0),
	mar = c(0, 0, 0, 0),
	mgp = c(2, 1, 0)
	)

legend.labels <- as.expression(lapply(lambdas, function(x) bquote(lambda==.(x))))

legend("center", ncol = length(lambdas), legend = legend.labels, lwd = 1.7, pch = pch, lty = lty)

# dev.off()

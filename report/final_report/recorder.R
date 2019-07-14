setwd("D:/")
a1 <- read.table("recorder.csv",header = F,sep=",")
attach(a1)
colnames(a1) = c("time","temp","hum","pressure","pm2.5","lx","isforce")
library(car)
aa = a1[,2:6]
scatterplotMatrix (aa, col = "black", pch = 20,cex = 0.5, smooth = FALSE,regLine = FALSE)
boxplot(aa)
library(psych)
m = fa.parallel(aa)
fa2 = fa(aa,nfactors = 2,fm='mle',rotate = 'varimax')
round(fa2$loadings,2)

aa = scale(aa)

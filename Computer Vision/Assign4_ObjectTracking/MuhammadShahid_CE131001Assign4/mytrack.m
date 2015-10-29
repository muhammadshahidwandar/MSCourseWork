clc
clear
frams = 40;
 I=rgb2gray(imread('stennis.40.ppm'));
I2=I(122:142,136:152);
temp =double(I2)/255;
for i = 21: frams
 fname=strcat('stennis.',int2str(i),'.ppm');
 k=imread(fname);
 imshow(k);
 drawnow;
end

for i = 21: frams
 fname=strcat('stennis.',int2str(i),'.ppm');
 I3=imread(fname);
 k=double(rgb2gray(I3))/255;
 %%%% I am just using cross correlation 
 Cxt = convn(k,temp,'same'); % Cross correlation
%  h = ones(size(temp));
%  Cgg = conv2(k,h,'same'); % finding the energy of signal
% NCC = Cxt./Cgg;
[a b] = max(max(Cxt));
[c d] = max(Cxt);
m = d(b);
n = b;
clear a b c d;
[a b] = size(k);
[c d] = size(temp);
c = round(c/2);
for i=1:a
    for j=1:b
        if (i==m-c && j>n-c && j<n+c)||(i==m+c && j>n-c && j<n+c)...
                || (j==n-c && i>m-c && i<m+c)||(j==n+c && i>m-c && i<m+c)
            I3(i,j) = 255;
            I3(i+1,j+1) = 255;
        end
    end
end

 imshow(I3);
 pause(0.25);
  
 drawnow;
end
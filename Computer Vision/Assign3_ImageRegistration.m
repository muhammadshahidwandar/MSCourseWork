clear all;
close all;
clc;
i = imread('reference.png');
x = double(rgb2gray(i))/255;



imshow(x); pause(1);
templt = imread('temp.png');
temp = double(rgb2gray(templt))/255;

imshow(temp); pause(1);

 CCR = conv2(x,temp,'same'); % Cross correlation
 g = ones(size(temp));
 Cgg = conv2(x,g,'same'); % finding the energy of signal
 NCC = CCR./Cgg;
 
[a b] = max(max(NCC));    %% to find Max in colomn
[c d] = max(NCC);
m = d(b);                  %%% max value in row #
n = b;                     %%% max value in colomn #
clear a b c d;
[a b] = size(x);
[c d] = size(temp);
c = round(c/2);
   %%% to draw the rectangle around template
for i=1:a
    for j=1:b
        if (i==m-c && j>n-c && j<n+c)||(i==m+c && j>n-c && j<n+c)...
                || (j==n-c && i>m-c && i<m+c)||(j==n+c && i>m-c && i<m+c)
            x(i,j) = 0;
            x(i+1,j+1) = 0;
        end
    end
end
imshow(x);
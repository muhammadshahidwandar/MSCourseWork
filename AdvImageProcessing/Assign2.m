%%%%Muhammad Shahid CE131001%%%%%%%%%%%%%%%%%%%%%%%
%%%Frequency Domain analysis of Gray Scale Image%%%
%%%%ADIP assignment#2%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
close all
clear all
clc
L=256;  % Gray Scal level of unsigned integer 8
%%%%%%%%%%%%%%%%%Step1%%%%%%%%%%%%%%%%%
Image = uint8(rgb2gray(imread('thumb3.jpg')));
I     = imresize(Image,[200 200]);
figure(1), imshow(I), colormap gray
title('Original image in Spatial Domain');
Ifft  = fftshift(fft2(double(I)));  %fourier transform of image
Ilog  = log(abs(Ifft)+1);           %log transform for visualization
figure(2), imshow(Ilog,[]), colormap gray
title('Fourier transform of gray scale Image');
%%%%%%%%%%%%%%%%%Step2%%%%%%%%%%%%%%%%%
%%%%%%%%%Band Pass Filter implementation from two High Pass Filters%%%
[m,n] = size(I);
filHpass10=ones(m,n);
filHpass20=ones(m,n);

 d60 = 45;
 d100= 95;
for i=1:m-1
      for j=1:n-1
          d1=((i-100)^2+(j-100)^2)^.5;
         if d1<=d60
             filHpass10(i,j) = 0;
         end   
         if d1<=d100 
             
             filHpass20(i,j)=0;
         end
         
      end
end
FilBandPass = filHpass10 - filHpass20;
figure(3), imshow(FilBandPass,[]), colormap gray
title('Band Pass Filter');

 G = FilBandPass.*Ifft;
 Imag = ifft2(fftshift(G)); % inverse fourier transform (freq domain to spatial domain)
 figure(4), imshow(abs(Imag),[ ]), colormap gray
 title('Band Pass Filtered in Frequency Domain Image');
 FiltSpatl = real(ifft2(fftshift(FilBandPass)));
 Filt  = FiltSpatl(1:5,1:5);
 Imag  = conv2(Filt,I);           % Applying fiter in spatial domain
 figure(5), imshow(Imag,[]), colormap gray
title('Band Pass Filter Image in Spatial Domain');
figure(6), imshow(FiltSpatl,[]), colormap gray
 

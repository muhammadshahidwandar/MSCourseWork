%%%%%%%%%%%%%%%%Muhammad Shahid CE131001%%%%%%%%%%%%%%%%%%%%%%%%%
%%image analysis using mathematical model,blurring due to motion%
%% and turbulance, image recovery using inverse filteration   %%%
%%%%%%%%%%%%%%%%%ADIP assignment#3%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
close all
clear all
clc
Image = uint8(imread('GW.jpg'));
I  = imresize(Image,[200 200]);
Ifft = fftshift(fft2(double(I)));
figure(1);
subplot(2,3,1);
imshow(I), colormap gray
title('Original Image');
% subplot(2,3,2)
% imshow(abs(Ifft),[24 100000]), colormap gray
% title('Original gray scale Image FFT');

%%%%%%%%%%%%%%%%%%%%%Blurring FIlter%%%%%%%%%%

%%%%Blurring Filter Modeling Due to Turbulance%%%%%%
%%
K  = 0.0025;
[m,n] = size(Ifft);
H_uv_T  = zeros(m,n); 
for u=1:m
      for v=1:n
      H_uv_T(u,v) = exp(-K*((u-100)^2+(v-100)^2)*5/6); %Blurr Filter in frequency domain 
      end
end
%subplot(2,2,3);
%imshow(H_uv_T), colormap gray


G_uv = H_uv_T.*Ifft;
% subplot(2,3,3);
% imshow(abs(G_uv),[24 100000]), colormap gray
% title('Blurred Image FFT due Turbulance');
g_uv  = ifft2(fftshift(G_uv));   %inverse fourier tranform for spatial domain
subplot(2,3,2);
imshow(abs(g_uv),[ ]), colormap gray
title('Blurred Due to Turbulance');
%%%%Blurring Filter Modeling Due to Motion%%%%%%
%%
T = 1;
a = 0.05  , b = 0.05; 
H_uv_M  = zeros(m,n);
pi = 3.14;
[m,n] = size(Ifft);
%Blurring filter due to motion in frequency domain
for u=1:m
      for v=1:n
          uc = u-100; vc = v-100;
          x  = pi*(a*uc+b*vc);
          if(uc+vc==0)
              H_uv_M(u,v) = 1;
          else
              H_uv_M(u,v) =(T/x)*(sin(x))*(exp(-1j*(x))) ; 
          end
      end
end
%imshow(abs(H_uv_M),[ ]), colormap gray 
 G_uv_M = H_uv_M.*Ifft;
% subplot(2,3,5);
% imshow(abs(G_uv_M),[24 100000]), colormap gray 
g_uv_M  = ifft2(fftshift(G_uv_M));
subplot(2,3,3);
imshow(abs(g_uv_M),[ ]), colormap gray 
title('Blurred Due to Motion');
 %%%%%Image Recovering after Motion Blure%%%%%%%%%
 %%%limited Inverse Filter%%%%%%%%%%%%%5
 %%
 H_uv_limit1 = ones(m,n);
 R = 80
 for u=1:m-1
      for v=1:n-1
        if  (sqrt((u-100)^2+(v-100)^2)<=R)
      H_uv_limit1(u,v) = H_uv_T(u,v); 
        end
      end
end
 F_hat_uv =  G_uv./H_uv_limit1;
% imshow(abs(F_hat_uv),[24 100000]), colormap gray
 f_hat_uv = ifft2(fftshift(F_hat_uv));
 subplot(2,3,4);
  imshow(abs(f_hat_uv),[ ]), colormap gray
  title('Restored using Limited Inverse Filter');
   %%%constrained least square Filter%%%%%%%%%%%%%5
 %%
 Gama = .01;
  H_uv_limit1 = ones(m,n);
  P_uv = zeros(m,n);
 for u=1:m-1
      for v=1:n-1
          P_uv(u,v) = -((u-100)^2+(v-100)^2)/(m*n); 
      end
      
 end
temp = (H_uv_T*G_uv);
  F_hat_uv_c =  (H_uv_T./(H_uv_T.^2+Gama.*P_uv.^2)).*G_uv;
% % imshow(abs(F_hat_uv),[24 100000]), colormap gray
  f_hat_uv_c = ifft2(fftshift(F_hat_uv_c));
  subplot(2,3,6);
  imshow(abs(f_hat_uv_c),[ ]), colormap gray
   title('Restored by constrained least square Filter Gama 0.01');


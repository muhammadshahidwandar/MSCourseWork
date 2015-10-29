%%%%Canny Edge Detector By Muahhamd Shahid CE131001
close all
clear all
clc
I=imread('faisalmosq.jpg');
I_gry=rgb2gray(I);
figure;
imshow(I_gry);
title('Orignal Aerial Gray Scal Image')
Sigma=15;
temp=Sigma*(sqrt(2*pi));
 for x=-18:9:18 
 Gx((x/9)+3)=exp(-((x*x)/(2*Sigma*Sigma)))./temp;        %%%Finding Guassian Value
 end
Gy=Gx';
 [R,C]=size(I_gry);                                      %%%As Guassian is Symetric So filter is separables in X and Y direction
 E=zeros(R-2,C-2);
 for n=1:R
     for m=1:C-5
         E(n,m)=sum(Gx.*double(I_gry(n,m:m+4)));            %%%Applying Guassian along X_axis 
    end
    
 end
 [R,C]=size(E);
 for m=1:C
     for n=1:R-5
         E(n,m)=sum(Gy.*double(E(n:n+4,m)));               %%%Applying Guassian along Y_axis 
        
    end
    
 end

%%%%%%%%%%%%%%%%%%%%%%%%


%Gy=Gx';
%filt_guss=1/80000.*[2 4 5 4 2;4 9 12 9 4;5 12 15 12 5; 4 9 12 9 4;2 4 5 4 2];
%  I_filt1= conv2(I_gry,Gx);
%  I_filt2= conv2(I_filt1,Gy);
 figure;
 imshow(E);
 title('Blurd Image after convolving with guassian filter')
 %%%%%%%%%%%%%Differential W.R.T X,Y%%%%%%%%%%%%%%%%%%%
 Dx=[-1 0 1];
 Dy=Dx';
[R,C] = size(E);
 for n=1:R
     for m=1:C-3
         Ix(n,m)=sum(Dx.*double(E(n,m:m+2)));        
    end
    
 end
 [R,C]=size(Ix);
 for m=1:C
     for n=1:R-3
         Iy(n,m)=sum(Dy.*double(E(n:n+2,m)));
        
    end
    
 end
 Ix=Ix(1:477,:);
 
 I_abs= abs(Ix)+abs(Iy);                    %%%%Finding absolut value 
  figure 
 imshow(I_abs);
 title('Asolute Magnitude of individual Gradiants')
 %%%%%%%%%%%%%%%%%%%Angle calculation%%%%%%%
 angl= atan2(Ix,Iy)*(180/pi);   %%%for calculation in degree multiply 180/pi
y = angl < 0;                 %%% shifting the value from negative to positive
angl = angl + 180*y;
% devide the angles into 4 directions
% 0-45 45-90 90-135 135-180

binDist =    [0 45 90 135 180];

[dummy, discrtAngl] = histc(angl,binDist);
%%%Making Zero in complete initial and final, Rows and colms 
discrtAngl(1,:)  = 0;
discrtAngl(end,:)=0;
discrtAngl(:,1)  = 0;
discrtAngl(:,end) = 0;
 
 
 
 
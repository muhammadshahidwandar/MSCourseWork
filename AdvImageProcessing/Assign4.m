%%%%%%%Assign#4 By Muhammad Shahid CE131001%%%%%
%%%image transformation and analysis%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
close all
clear all
clc
I = uint8(imread('RGBimage.jpg'));
figure(1),imshow(I);
title('Orignal RGB Image');
pi = 3.14;
R_theta_dgry = 45;  %45 deg Anti Clock wise rotation
R_theta_rdan = pi*R_theta_dgry/180; %angle in radian
sin_t = sin(R_theta_rdan); 
cos_t  = cos(R_theta_rdan);
[m,n,c]  = size(I);
d  = ceil(sqrt(m^2+n^2));  %diagonel Length
x_centr = ceil((d+1)/2);   % center coordinates of image in x
y_centr = ceil((d+1)/2);   % center coordinates of image in y
pad_x     = ceil((d-m)/2); % pad image upto diagonal size in x
pad_y     = ceil((d-n)/2); % pad image upto diagonal size in y
Imag_pad= padarray(I,[pad_x pad_y]); % image padding
Imag_rotat = uint8(zeros(m+2*pad_x,n+2*pad_y,c)); 
figure(2), imshow(Imag_pad);
title('Diagonal Size zero padded Image');
[l,p,c] = size(Imag_rotat);
%R_mat =   [cos_t -sin_t 0; sin_t cos_t 0; 0 0 1];
%image rotation along an angle
for i = 1:l-1
    for j = 1:p-1
        x  = (i-x_centr)*cos_t+(j-y_centr)*sin_t;
        y  = -(i-x_centr)*sin_t+(j-y_centr)*cos_t;
        x  =round(x)+x_centr;
        y  =round(y)+y_centr;
        for k =1:c
       if (x>=1 && y>=1 && x<=l && y<=p)
              Imag_rotat(i,j,k)=Imag_pad(x,y,k); % k degrees rotated image         
       end
        end
    end
end
figure(3), imshow(Imag_rotat);
title('Rotated Image');
%%%%%%%%%%Part 2 Image Scaling%%%%%%%%%%
[m,n,c] = size(Imag_rotat);
Sx = 2; % scale along x
Sy = 2; % scale along y
%%
Imag_scal = uint8(zeros(Sx*m,Sy*n,c));
for i = 1:Sx*m -2
    for j = 1:Sy*m-2           %%%%Scaling by Bilinear Interpolation Mathod
        x_cl  = ceil(i/Sx);
        y_cl  = ceil(j/Sy);
        Del_x = x_cl - i/Sx;
        Del_y = y_cl - j/Sy;
        for k =1:c
        A =Imag_rotat(x_cl,y_cl,k); B =Imag_rotat(x_cl,y_cl+1,k);
        C =Imag_rotat(x_cl+1,y_cl,k); D =Imag_rotat(x_cl+1,y_cl+1,k);
        
        Imag_scal(i,j,k)= (1-Del_x)*(1-Del_y)*A+ (Del_x)*(1-Del_y)*B+.....
                         (1-Del_x)*(Del_y)*C+Del_x*Del_y*D;
        end  
    end
end
figure(4), imshow(Imag_scal);
title('Scaled By 2 Image');
%%%%%%%%%%%%%%%%%%%%%%%%Part 3 HSI Contrast streching%%%%%%%%%%%%%%
%HSI Conversion
%%
Imag = double(Imag_scal)/255;
r=Imag(:,:,1);
g=Imag(:,:,2);
b=Imag(:,:,3);
nom=1/2*((r-g)+(r-b));
denom=((r-g).^2+((r-b).*(g-b))).^0.5;

% avoid zero exception
h=acosd(nom./(denom+0.000001));
%If b>g then h= 360-angle
h(b>g)=360-h(b>g);

%normalize to the range [0 1]
h=h/360;
%saturation
s=1- (3./(sum(Imag,3)+0.000001)).*min(Imag,[],3);
%Intensity
i=sum(Imag,3)./3;
%HSI
hsi=zeros(size(Imag));
hsi(:,:,1)=h;
hsi(:,:,2)=s;
hsi(:,:,3)=i;
%%%%%%%%%%%%%%%%%%% Intensity streching in HSI%%%%%%%%%%
%%%Globel Contrast streching%%%%%%%%%%
Level = 1;
Imin= min(min(i));
Imax= max(max(i));
i_contrs=((Level)/(Imax-Imin)).*(i-Imin); 
hsi(:,:,3)=i_contrs;
figure(5)
imshow(hsv2rgb(hsi));
title('Contrast Streched HSV2RGB Image');
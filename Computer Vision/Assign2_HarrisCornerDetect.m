clc
clear all
close all
I=imread('faisalmosq.jpg');
Igry = rgb2gray(I);
figure
imshow(Igry);
title('original grayscal image')
%%%%%%%%%%%%%%%%%%%%%%%%%%guassan+diff%%%%%%%%%%
sig=2;
x = floor(-2*sig):ceil(2*sig);
G = exp(-0.5*x.^2/sig^2);
G = G/sum(G);
%%%dx=[-1 0 1;-1 0 1;-1 0 1];
dx=[-1 0 1];

dy = dx';
Ix=conv2(Igry,dx,'same');
Iy=conv2(Igry,dy,'same');
Ixy=Ix.*Iy;

Ixy_gs=conv2(Ixy,G);
Ix2  = conv2(Ix.^2,G);
Iy2  = conv2(Iy.^2,G);
%Ix_sqrt=Ix.*Ix;
%Iy_sqrt=Iy.*Iy;
figure
imshow(Ixy_gs);
%%%%%%%%%%%%%%%%%%%%%%%%%%%
k=0.04;
 R11 = (Ix2.*Iy2 - Ixy_gs.^2) - k.*((Ix2 + Iy2).^2);
 
  R11=(255/max(max(R11)))*R11;    %%%normalizing the value means bounding it to 255
  figure
  imshow(R11);
  title('R response')
    R=R11;
    K=20;
    me=mean(mean(R)); 
    T=me+k*sig;
    R(find(R<T))=0 ;    %%%%suppressing the value with low corner response
    figure
    imshow(R);
    title('low Response value suppressing')
    %%%%%local maxima selection 
    [Row,Col]=size(R);
    R_max=zeros(Row,Col);
    for(i=1:10:Row-10)
        for(j=1:10:Col-10)
            Temp=R(i:i+10,j:j+10)
            v =max(max(Temp))
            R_max(find(R(i:i+10,j:j+10)==v))=255;
        end
       
    end
    
    
    
    
    
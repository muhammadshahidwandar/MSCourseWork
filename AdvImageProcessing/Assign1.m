%%%%Muhammad Shahid CE131001%%%%%%%%%%%%%%%%
%%%%ADIP assignment#1%%%%%%%%%%%%%%%%%%%%%%
close all
clear all
clc
Level = 256;   % Gray Scal level for contrast streching
Image = uint8(imread('rizwan.jpg')); %read an rgb image in unsigned 8 bit 
I  = imresize(Image,[500 500]);
figure(1);
imshow(I);
title('Original RGB Image')
I_gry= (0.2.*I(:,:,1)+0.7.*I(:,:,2)+0.1.*I(:,:,3))/3;  % Gray scale image from R, G, B channel
%I_gry= (I(:,:,1)+I(:,:,2)+I(:,:,3))/3; % intensity image from R, G, B channel
figure(2);
imshow(I_gry);
title('Original Grey Scale Image')
%%%globel Contrast streching%%%%%%%%%%
Imin= min(min(I_gry));
Imax= max(max(I_gry));
I_gry_Glblcontrs=((Level-1)/(Imax-Imin)).*(I_gry-Imin);% globel contrast streching 
figure(3)
imshow(I_gry_Glblcontrs);
title('Globel Contrast streching grey scal Image')
%%%%%%%Local Contras streching overlaping and non overlaping%%%%%%%%%%%
[numrow,numcol] =size(I_gry);
I_gry_loclnon = zeros(numrow,numcol);
%%%%%%%non overlaping %%%%%%%%%%%%%%%
subsize=25;
 for row = 1:subsize:numrow+1-subsize
 for col = 1:subsize:numcol+1-subsize
    subImag = I_gry(row:row+subsize-1,col:col+subsize-1);
    mean = (1/(subsize*subsize))*sum(sum(subImag));
    for i=1:subsize
    for j= 1:subsize
            temp=(subImag(i,j)-mean);
       if(temp<20)
         I_gry_loclnon(i+row,j+col)=  2*temp +mean;
       else
       
         I_gry_loclnon(i+row,j+col) = subImag(i,j);
       end
        end
        end
    
 end
 end
 figure(4)
 imagesc(I_gry_loclnon);
 colormap(gray);
 title('Local nonoverlaped Contrast streching grey scal Image');
%%%%%%% overlaping %%%%%%%%%%%%%%%
I_gry_loclover = zeros(numrow,numcol);

subsize=25;
 for row = 1:numrow+1-subsize
 for col = 1:numcol+1-subsize
    subImag = I_gry(row:row+subsize-1,col:col+subsize-1);
    mean = (1/(subsize*subsize))*sum(sum(subImag));
    temp=(subImag(13,13)-mean);
       if(temp<30)
         I_gry_loclover(12+row,12+col)=  2*temp +mean;
       else
       
         I_gry_loclover(12+row,12+col) = subImag(13,13);
       end
             
    
 end
 end
 figure(5)
 imagesc(I_gry_loclover);
 colormap(gray);
title('Local Overlaped Contrast streching grey scal Image');
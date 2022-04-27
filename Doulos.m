%%

clc; clear; close all; 

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Organoid FLIM Analysis code
% 
% v220317 drafted
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% options
image_view = true; % true : image drawing, false : image non-drawing                  
is_write = true; % true : image writing, false : image non-writing
is_medfilt = true; % true: image medfilt, false: image non-medfilt

%% INI Parameters

% Filename
filelist = dir;
for i = 1 : length(filelist)
    if (length(filelist(i).name) > 5)
        if strcmp(filelist(i).name(end-4:end),'.data')
            dfilenm = filelist(i).name(1:end-5);
        end
    end
end
clear filelist;

% Please check ini file in the folder
fid = fopen(strcat(dfilenm,'.ini'));
config = textscan(fid,'%s');
fclose(fid); 

flimIntensityRange = zeros(4,2);
flimLifetimeRange = zeros(4,2);
for i = 1 : length(config{:})
    
    for j = 1 : 4
        if (strfind(config{1}{i},sprintf('flimIntensityRangeMax_Ch%d',j)))
            eq_pos = strfind(config{1}{i},'=');
            flimIntensityRange(j,2) = str2double(config{1}{i}(eq_pos+1:end));
        end
        if (strfind(config{1}{i},sprintf('flimIntensityRangeMin_Ch%d',j)))
            eq_pos = strfind(config{1}{i},'=');
            flimIntensityRange(j,1) = str2double(config{1}{i}(eq_pos+1:end));
        end  
        if (strfind(config{1}{i},sprintf('flimLifetimeRangeMax_Ch%d',j)))
            eq_pos = strfind(config{1}{i},'=');
            flimLifetimeRange(j,2) = str2double(config{1}{i}(eq_pos+1:end));
        end
        if (strfind(config{1}{i},sprintf('flimLifetimeRangeMin_Ch%d',j)))
            eq_pos = strfind(config{1}{i},'=');
            flimLifetimeRange(j,1) = str2double(config{1}{i}(eq_pos+1:end));
        end    
    end
    
    if (strfind(config{1}{i},'flimLifetimeColorTable'))
        eq_pos = strfind(config{1}{i},'=');
        flimLifetimeColorTable = str2double(config{1}{i}(eq_pos+1:end));
    end    
     
    if (strfind(config{1}{i},'nLines'))
        eq_pos = strfind(config{1}{i},'=');
        pixelsize = str2double(config{1}{i}(eq_pos+1:end));
    end 
    if (strfind(config{1}{i},'imageAveragingFrames'))
        eq_pos = strfind(config{1}{i},'=');
        imageAveragingFrames = str2double(config{1}{i}(eq_pos+1:end));
    end
    
    if (strfind(config{1}{i},'imageStichingXStep'))
        eq_pos = strfind(config{1}{i},'=');
        imageStichingXStep = str2double(config{1}{i}(eq_pos+1:end));
    end
    if (strfind(config{1}{i},'imageStichingYStep'))
        eq_pos = strfind(config{1}{i},'=');
        imageStichingYStep = str2double(config{1}{i}(eq_pos+1:end));
    end
    if (strfind(config{1}{i},'imageStichingMisSyncPos'))
        eq_pos = strfind(config{1}{i},'=');
        imageStichingMisSyncPos = str2double(config{1}{i}(eq_pos+1:end));
    end   
end

% Parameters (Size, imageNumber)
imageSize = pixelsize * pixelsize;
imageNumber = imageStichingXStep * imageStichingYStep;
imageNumber = 1;

% adding file
if (is_write), mkdir('scaled_image_matlab'); end

clear config eq_pos;

%% Colortable

fname_imap = 'C:\Users\Organoid PC\Documents\MATLAB\ColorTable\fire.colortable';
fname_imap_ch1 = 'C:\Users\Organoid PC\Documents\MATLAB\ColorTable\flim_ch1.colortable'; % C:\Users\BOP\Documents\MATLAB\ColorTable
fname_imap_ch2 = 'C:\Users\Organoid PC\Documents\MATLAB\ColorTable\flim_ch2.colortable';
fname_imap_ch3 = 'C:\Users\Organoid PC\Documents\MATLAB\ColorTable\flim_ch3.colortable';
fname_lmap = strcat('C:\Users\Organoid PC\Documents\MATLAB\ColorTable\hsv1.colortable');
fname_irmap = 'C:\Users\Organoid PC\Documents\MATLAB\ColorTable\jet.colortable';

fid = fopen(fname_imap,'r');
imap = reshape(fread(fid,'uint8'),[256 3])/255;
fclose(fid);

fid = fopen(fname_imap_ch1,'r');
imap_ch1 = reshape(fread(fid,'uint8'),[256 3])/255;
fclose(fid);

fid = fopen(fname_imap_ch2,'r');
imap_ch2 = reshape(fread(fid,'uint8'),[256 3])/255;
fclose(fid);

fid = fopen(fname_imap_ch3,'r');
imap_ch3 = reshape(fread(fid,'uint8'),[256 3])/255;
fclose(fid);

fid = fopen(fname_lmap,'r');
lmap = reshape(fread(fid,'uint8'),[256 3])/255;
fclose(fid);

fid = fopen(fname_irmap,'r');
irmap = reshape(fread(fid,'uint8'),[256 3])/255;
fclose(fid);

clear fid color_list fname_imap fname_lmap fname_irmap;                

%% FLIM Image

intensity_image_raw = zeros(pixelsize,pixelsize,4);
lifetime_image_raw = zeros(pixelsize,pixelsize,4);
merged_image_raw = zeros(pixelsize,pixelsize,3,4);

flimIntensityRange = [0,0.3; 0,0.2; 0,0.1; 0,0.2]; % 3 x 2
flimLifetimeRange = [1.0,6.5; 1.0,6.5; 1,5.5; 0.5,2.0]; % 3 x 2 

% load data
fname = sprintf([dfilenm,'.data']);
fid = fopen(fname,'r');

for n = 1 : imageNumber
    
    for i = 1 : 4
        temp = fread(fid,imageSize,'float');
        intensity_image_raw(:,:,i,n) = reshape(temp,[pixelsize pixelsize])';
        if (is_medfilt)
            intensity_image_raw(:,:,i,n) = medfilt2(intensity_image_raw(:,:,i,n),[3 3],'symmetric');
        end
        intensity_image(:,:,i,n) = intensity_image_raw(imageStichingMisSyncPos+1:end,:,i,n);
    end

    for i = 1 : 4
        temp = fread(fid,imageSize,'float');
        lifetime_image_raw(:,:,i,n) = reshape(temp,[pixelsize pixelsize])';
        if (is_medfilt)
            lifetime_image_raw(:,:,i,n) = medfilt2(lifetime_image_raw(:,:,i,n),[3 3],'symmetric');
        end
        lifetime_image(:,:,i,n) = lifetime_image_raw(imageStichingMisSyncPos+1:end,:,i,n);
    end
    
    for i = 1 : 4
        temp_l = uint8(255*mat2gray(lifetime_image_raw(:,:,i,n),[flimLifetimeRange(i,1) flimLifetimeRange(i,2)]));    
        temp_l = ind2rgb(temp_l,lmap);
        temp_i = mat2gray(intensity_image_raw(:,:,i,n),[flimIntensityRange(i,1) flimIntensityRange(i,2)]);  
        merged_image_raw(:,:,:,i,n) = temp_l .* repmat(temp_i,[1 1 3]);
        merged_image(:,:,:,i,n) = merged_image_raw(imageStichingMisSyncPos+1:end,:,:,i,n);
    end
end

fclose(fid);
save('result.mat','intensity_image','lifetime_image');
clear fid;

%% Visualization
  
if (image_view)

    figure(100);
    subplot(341); imagesc(intensity_image(:,:,1,1)); axis image; 
    caxis([flimIntensityRange(1,1) flimIntensityRange(1,2)]);
    colormap(imap_ch3); freezeColors;
    title(sprintf('Ch1 Intensity [%.1f %.1f]',flimIntensityRange(1,1),flimIntensityRange(1,2)));
    subplot(342); imagesc(intensity_image(:,:,2,1)); axis image;
    caxis([flimIntensityRange(2,1) flimIntensityRange(2,2)]);
    colormap(imap_ch3); freezeColors;
    title(sprintf('Ch2 Intensity [%.1f %.1f]',flimIntensityRange(2,1),flimIntensityRange(2,2)));
    subplot(343); imagesc(intensity_image(:,:,3,1)); axis image;
    caxis([flimIntensityRange(3,1) flimIntensityRange(3,2)]);
    colormap(imap_ch3); freezeColors;
    title(sprintf('Ch3 Intensity [%.1f %.1f]',flimIntensityRange(3,1),flimIntensityRange(3,2)));
    subplot(344); imagesc(intensity_image(:,:,4,1)); axis image;
    caxis([flimIntensityRange(4,1) flimIntensityRange(4,2)]);
    colormap(imap_ch3); freezeColors;
    title(sprintf('Ch4 Intensity [%.1f %.1f]',flimIntensityRange(4,1),flimIntensityRange(4,2)));
    
    subplot(345); imagesc(lifetime_image(:,:,1,1)); axis image;
    caxis([flimLifetimeRange(1,1) flimLifetimeRange(1,2)]);
    colormap(lmap); freezeColors; 
    title(sprintf('Ch1 Lifetime [%.1f %.1f]',flimLifetimeRange(1,1),flimLifetimeRange(1,2)));
    subplot(346); imagesc(lifetime_image(:,:,2,1)); axis image;
    caxis([flimLifetimeRange(2,1) flimLifetimeRange(2,2)]);
    colormap(lmap); freezeColors; 
    title(sprintf('Ch2 Lifetime [%.1f %.1f]',flimLifetimeRange(2,1),flimLifetimeRange(2,2)));
    subplot(347); imagesc(lifetime_image(:,:,3,1)); axis image;
    caxis([flimLifetimeRange(3,1) flimLifetimeRange(3,2)]);
    colormap(lmap); freezeColors; 
    title(sprintf('Ch3 Lifetime [%.1f %.1f]',flimLifetimeRange(3,1),flimLifetimeRange(3,2)));
    subplot(348); imagesc(lifetime_image(:,:,4,1)); axis image;
    caxis([flimLifetimeRange(4,1) flimLifetimeRange(4,2)]);
    colormap(lmap); freezeColors; 
    title(sprintf('Ch4 Lifetime [%.1f %.1f]',flimLifetimeRange(4,1),flimLifetimeRange(4,2)));

    subplot(3,4,9); imagesc(merged_image(:,:,:,1,1)); axis image; title('Ch1 Merged');
    subplot(3,4,10); imagesc(merged_image(:,:,:,2,1)); axis image; title('Ch2 Merged');
    subplot(3,4,11); imagesc(merged_image(:,:,:,3,1)); axis image; title('Ch3 Merged');
    subplot(3,4,12); imagesc(merged_image(:,:,:,4,1)); axis image; title('Ch4 Merged');

end


%% Intensity ratio

% % load data
% fname = sprintf([dfilenm,'.data']);
% fid = fopen(fname,'r');
% 
% for i = 1 : 3
%     temp = fread(fid,frameSize,'float');
%     intensity_image(:,:,i) = reshape(temp,[imageSize imageSize])';
%     if (is_medfilt)
%         intensity_image(:,:,i) = medfilt2(intensity_image(:,:,i),[3 3],'symmetric');
%     end
% end
% 
% fclose(fid);
% clear fid;

IntRatiorange = [0 0.8; 2 16; 0 12.5];
% 
% for n = 1 : imageNumber
%     Ch1_Ch2_ratio(:,:,n) = intensity_image(:,:,1,n)./intensity_image(:,:,2,n);
%     Ch2_Ch3_ratio(:,:,n) = intensity_image(:,:,2,n)./intensity_image(:,:,3,n);
%     Ch1_Ch3_ratio(:,:,n) = intensity_image(:,:,1,n)./intensity_image(:,:,3,n);
% end
% 
% figure(50);
% subplot(131); imagesc(Ch1_Ch2_ratio(:,:,2)); caxis(IntRatiorange(1,:)); colormap jet; axis image;
% subplot(132); imagesc(Ch2_Ch3_ratio(:,:,2)); caxis(IntRatiorange(2,:)); colormap jet; axis image;
% subplot(133); imagesc(Ch1_Ch3_ratio(:,:,2)); caxis(IntRatiorange(3,:)); colormap jet; axis image;


%% Image Write

if (is_write)
    
    for n = 1 : imageNumber
        for i = 1 : 4
            temp = uint8(255 * mat2gray(intensity_image(:,:,i,imageNumber-n+1),[flimIntensityRange(i,1) flimIntensityRange(i,2)])); 
            imwrite(temp,imap,sprintf('scaled_image_matlab/intensity_image_ch_%d_avg_%d_[%.1f %.1f]_%d.bmp',...
                i,imageAveragingFrames,flimIntensityRange(i,1),flimIntensityRange(i,2),n));
        end
        for i = 1 : 4
            temp = uint8(255 * mat2gray(lifetime_image(:,:,i,imageNumber-n+1),[flimLifetimeRange(i,1) flimLifetimeRange(i,2)])); 
            imwrite(temp,lmap,sprintf('scaled_image_matlab/lifetime_image_ch_%d_avg_%d_[%.1f %.1f]_%d.bmp',...
                i,imageAveragingFrames,flimLifetimeRange(i,1),flimLifetimeRange(i,2),n));
        end
        for i = 1 : 4
            temp = uint8(255 * merged_image(:,:,:,i,imageNumber-n+1));
            imwrite(temp,sprintf('scaled_image_matlab/merged_image_ch_%d_avg_%d_i[%.1f %.1f]_l[%.1f %.1f]_%d.bmp',...
                i,imageAveragingFrames,flimIntensityRange(i,1),flimIntensityRange(i,2),...
                flimLifetimeRange(i,1),flimLifetimeRange(i,2),n));
        end

%         temp = uint8(255 * mat2gray(Ch1_Ch2_ratio(:,:,imageNumber-n+1),[IntRatiorange(1,1) IntRatiorange(1,2)]));
%         imwrite(temp,irmap,sprintf('scaled_image_matlab/intensity_ratio_Ch1_Ch2[%.1f %.1f]_%d.bmp',...
%         IntRatiorange(1,1),IntRatiorange(1,2),n));
% 
%         temp = uint8(255 * mat2gray(Ch2_Ch3_ratio(:,:,imageNumber-n+1),[IntRatiorange(2,1) IntRatiorange(2,2)]));
%         imwrite(temp,irmap,sprintf('scaled_image_matlab/intensity_ratio_Ch2_Ch3[%.1f %.1f]_%d.bmp',...
%         IntRatiorange(2,1),IntRatiorange(2,2),n));
% 
%         temp = uint8(255 * mat2gray(Ch1_Ch3_ratio(:,:,imageNumber-n+1),[IntRatiorange(3,1) IntRatiorange(3,2)]));
%         imwrite(temp,irmap,sprintf('scaled_image_matlab/intensity_ratio_Ch1_Ch3[%.1f %.1f]_%d.bmp',...
%         IntRatiorange(3,1),IntRatiorange(3,2),n));
    end

end

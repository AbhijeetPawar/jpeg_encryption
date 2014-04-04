#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <jerror.h>
#include <setjmp.h>
#include <math.h>
#include <assert.h>

int encode = 1;

struct Image
{
	unsigned char *image_buffer;	//JSAMPLE *image_buffer;
	long image_width;
	long image_height;
	
}my_image;

int read_JPEG(char *filename)
{
	struct jpeg_decompress_struct cinfo;
	
	struct jpeg_error_mgr jerr;
	
	FILE *infile;
	
	JSAMPARRAY buffer;
	int row_stride,count;
	
	if((infile=fopen(filename,"rb"))==NULL)
	{
		fprintf(stderr,"\nCannot open input file %s\n",filename);
		return 0;
	}
	
	cinfo.err = jpeg_std_error(&jerr); 		//(&jerr); default
	/*jerr.pub.err_exit = my_error_exit;	//to override default exit
	
	if(setjmp(jerr.setjmp_buffer))
	{
		jpeg_distroy_decompress(&cinfo);
		fclose(infile);
		return 0;
	}
	*/
	jpeg_create_decompress(&cinfo);
	
	jpeg_stdio_src(&cinfo,infile);
	
	jpeg_read_header(&cinfo,TRUE);
	
	jpeg_start_decompress(&cinfo);
	
	row_stride = cinfo.output_width * cinfo.output_components;

	my_image.image_height = cinfo.output_height;
	my_image.image_width  = cinfo.output_width;
	my_image.image_buffer = malloc(cinfo.output_width*cinfo.output_height*cinfo.output_components);
	count = 0;
	
	buffer = (* cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo,JPOOL_IMAGE,row_stride,1);
	//buffer[0] = (JSAMPROW)malloc(sizeof(JSAMPLE) * row_stride);				
	
	while(cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines(&cinfo,buffer,1);
		memcpy(my_image.image_buffer+count,buffer[0],row_stride);
		count += row_stride;
	}
	
	jpeg_finish_decompress(&cinfo);
	
	jpeg_destroy_decompress(&cinfo);
	
	fclose(infile);

	return 1;
}

int *magicsq(int n)
{
	int **a,*b;
	int i,j,k,ip,jp,num;
	
	a = NULL;
	a = (int **)malloc(sizeof(int *) * n);
	assert(a!=NULL);
	
	for(i=0;i<n;i++)
		a[i] = calloc(n,sizeof(int));
	
	i=0;
	j=(n-1)/2;
	a[i][j]=1;

	for(num=2;num<=(n*n);num++)
	{	
		i--;
		j--;
		
		if(i<0)
			i=n-1;
		if(j<0)
			j=n-1;
		
		if(a[i][j] == 0) {	
			a[i][j]=num;
			ip=i;
			jp=j;
		} else {
			k=ip+1;
			if(k >= n)
				k=0;
				
			a[k][jp]=num;
			i=k;
			j=jp;
		}
	}
	
	b = NULL;
	b = calloc(n*n,sizeof(int));
	assert(b!=NULL);
	
	k=0;
	for(i=0;i<n;i++)
		for(j=0;j<n;j++)
			b[k++] = a[i][j];

	for(i=0;i<n;i++)
		free(a[i]);
	free(a);
	
	return b;
}

void process()
{
	int count = my_image.image_width*my_image.image_height*3;
	int *a,c,i,j;
	unsigned char *buff = my_image.image_buffer;
	unsigned char temp;
	
	c = sqrt(count);
	if(c%2==0)	//we need odd no for magic sq!
		c--;
		
	a = magicsq(c);
	
	if(encode) {
		for (i = 0; i < (c*c);i += 2) {
			j = a[i];
			
			temp = buff[i];
			buff[i] = buff[j];
			buff[j] = temp;
		}
		for (i = 1;i < (c*c);i += 2) {
			j = a[i];
			
			temp = buff[i];
			buff[i] = buff[j];
			buff[j] = temp;
		}
	} 
	else {		
		for (i = (c*c)-2; i > 0; i -= 2) {
			j = a[i];
			
			temp = buff[i];
			buff[i] = buff[j];
			buff[j] = temp;
		}		
		for (i = (c*c)-1; i >= 0; i -= 2) {
			j = a[i];
			
			temp = buff[i];
			buff[i] = buff[j];
			buff[j] = temp;
		}
	}
	free(a);
}

void _process()
{
	int count = my_image.image_width*my_image.image_height*3;
	int i,j;
	unsigned char *buff = my_image.image_buffer;
	unsigned char temp,temp1;
	
	while(count)
	{
		temp = *buff;
		temp1 = 0;
		
		for(i=0;i<8;i++)
		{
			temp1 = temp1 << 1;
			
			if(temp & 0x1)
				temp1 = temp1 | 0x1;		
			
			temp = temp >> 1;
		}
		*buff = temp1;
		
		buff++;
		count--;
	}
}


int write_JPEG(char *filename)
{
	struct jpeg_compress_struct cinfo;
	
	struct jpeg_error_mgr jerr;
	
	FILE *outfile;
	JSAMPROW row_pointer[1];
	int row_stride;
	
	cinfo.err = jpeg_std_error(&jerr);
	
	jpeg_create_compress(&cinfo);
	
	if ((outfile = fopen(filename, "wb")) == NULL) 
	{
		fprintf(stderr, "can't open %s\n", filename);
		exit(1);
	}

	jpeg_stdio_dest(&cinfo,outfile); 

	cinfo.image_width = my_image.image_width;
	cinfo.image_height = my_image.image_height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);

	jpeg_set_quality(&cinfo, 100, TRUE);

	jpeg_start_compress(&cinfo,TRUE);

	row_stride = my_image.image_width * 3;

	while(cinfo.next_scanline < cinfo.image_height)
	{
		row_pointer[0] = &my_image.image_buffer[cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1); 
	}
	
	jpeg_finish_compress(&cinfo);
	
	fclose(outfile);
	
	jpeg_destroy_compress(&cinfo);
	
	return 1;
}

int main(int *argc, char **argv)
{
	char c = 'e';

	if(argc < 2) {
		printf("\nInvalid no of arguements! Exit!");
		return 0;
	}
	strcpy(infile, argv[1]);
	strcpy(outfile, argv[2]);
	
	printf("\nENCODE / DECODE ? (e/d) : ");
	scanf("%c",&c);
	
	if(c=='d')
		encode = 0;
	
	read_JPEG(infile);
	
	process();
	//_process();
	
	write_JPEG(outfile);
	
	return 0;
}

#!/usr/bin/perl
# perl rg_conv2.pl ~/2510/linux-2.6.30_hwnat/drivers/net/rtl86900/romeDriver/rtk_rg_define.h > rg_conv.pl 

printf("#!/usr/bin/perl\n");
printf("while (<>) {\n");
printf("	\$line=\$_;\n");

while (<>) {
	$line=$_;
	
	if ($line=~/#define (rtk_rg[a-zA-Z_0-9]+)\(.*?\) +(rtk_[a-zA-Z_0-9]+)\(.*?\)/) {
		printf("	\$line =~ s/$2 *\\(/$1\\(/g;\n");
	}
}

printf("	print \$line;\n");
printf("}\n");


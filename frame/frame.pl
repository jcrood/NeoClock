#!/usr/bin/perl

use Modern::Perl;
use PDF::API2;

use constant mm => 25.4 / 72;

my $rim_radius = 1000 / 3.1415 / 2;
my $bar_width = 30;
my $wall_thickness = 3;
my $overhang = 10;

my $minAngle = 135;
my $maxAngle = 225;

my $pdf = PDF::API2->new;
$pdf->mediabox('A4');
my $page = $pdf->page;
my $gfx = $page->gfx;

my $centerX = 200/mm;
my $centerY = (297/2)/mm;


# overhang
$gfx->arc($centerX, $centerY,
    ($rim_radius + $overhang)/mm,
    ($rim_radius + $overhang)/mm,
    $minAngle, $maxAngle, 1);
$gfx->stroke;

# outer wall
$gfx->arc($centerX, $centerY,
    $rim_radius/mm,
    $rim_radius/mm,
    $minAngle, $maxAngle, 1);
$gfx->stroke;

$gfx->arc($centerX, $centerY,
    ($rim_radius - $wall_thickness)/mm,
    ($rim_radius - $wall_thickness)/mm,
    $minAngle, $maxAngle, 1);
$gfx->stroke;

# inner wall
$gfx->arc($centerX, $centerY,
    ($rim_radius - $bar_width + $wall_thickness)/mm,
    ($rim_radius - $bar_width + $wall_thickness)/mm,
    $minAngle, $maxAngle, 1);
$gfx->stroke;

$gfx->arc($centerX, $centerY,
    ($rim_radius - $bar_width)/mm,
    ($rim_radius - $bar_width)/mm,
    $minAngle, $maxAngle, 1);
$gfx->stroke;

$gfx->move($centerX - 120/mm, $centerY + 120/mm);
$gfx->line($centerX, $centerY);
$gfx->line($centerX - 120/mm, $centerY - 120/mm);
$gfx->stroke();

$pdf->saveas("frame.pdf")

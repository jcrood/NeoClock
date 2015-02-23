// NeoClock frame
// John Rood, 2015-02-02

include <wedge.scad>

// since it's quite large, smooth it out
$fa = 1;
$fs = 0.5;

// 1 meter circumference
rim_radius = 1000 / PI / 2; // <----  SUBSTRACT A COUPLE OF TENTHS OR MORE FOR SLOPPY PRINTERS
num_sections = 6; // Keep this an even number if you know what's right
wall_thickness = 3; // both wall and frontface thickness
// beware, total width = bar + overhang
bar_width = 30;
overhang = 10;
ribbon_width = 10; // width of LED pcb
mount_width = 15;
diffuser_width = 22;
extrusion_width = 0.66; // used for diffuser wall width


section_angle = 360 / num_sections;
width = ribbon_width + wall_thickness + wall_thickness;
section_hole = 3.5; // does this fit m3?
explodeSpace = 20;
mounthole_offset = wall_thickness - 2;


module connector(sectionNo, cwidth = 9, thickness = 10, cheight = 10)
{
    offset = 1.5;
    rotate([0, 0, sectionNo * section_angle])
    translate([-thickness/2, rim_radius - bar_width + 1, 0]) {
        difference() {
            union() {
                translate([0, offset, 0])
                    cube([thickness, cwidth, cheight + wall_thickness + wall_thickness/2]);
                translate([0, bar_width - 2 - cwidth - offset, 0])
                    cube([thickness, cwidth, cheight + wall_thickness + wall_thickness/2]);
                //top
                translate([0, 0, cheight + wall_thickness])
                    cube([thickness , bar_width - 2, wall_thickness]);
            }
            translate([-1, thickness / 2 + offset, cheight  / 2 + wall_thickness])
                rotate([0, 90, 0])
                    cylinder(r = section_hole / 2, h = thickness + 2);
            translate([-1, bar_width - 2 - thickness / 2 - offset, cheight  / 2 + wall_thickness])
                rotate([0, 90, 0])
                    cylinder(r = section_hole / 2, h = thickness + 2);
        }
    }
}

module mount(big_hole = 6, small_hole = 4)
{
    difference() {
        translate([0, 1, 0]) cube([mount_width, bar_width - 2, wall_thickness]);

        translate([mount_width/2, wall_thickness + big_hole/2 + 1 + mounthole_offset, -1])
            cylinder(r = big_hole/2, h = wall_thickness + 2);

        translate([mount_width/2 - small_hole/2, wall_thickness + big_hole - 1 + mounthole_offset, -1]) {
            cube([small_hole, big_hole, wall_thickness + 2]);
            translate([small_hole/2, big_hole, 0])
            cylinder(r = small_hole/2, h = wall_thickness + 2);
        }
    }
    // hole bridge
    translate([0, 1, 0]) cube([mount_width, bar_width - 2, 0.55]);
}

module ldr()
{
    rotate([0, 90, 0])
    intersection() {
        cylinder(r=2.6, h = 10);
        translate([-3, -2.2, -1]) cube([6, 4.4, 12]);
    }
}


module cable(width = 6.0, height = 3.0, length = 10)
{
    translate([-width/2 + height/2, -length/2, -height/2])
    rotate([-90,0,0])
    color([1.0, 1.0, 1.0, 1.0]) union() {
         cylinder(r=height/2, h = length);
        translate([width - height, 0, 0]) cylinder(r=height/2, h = length);
        translate([0, -height/2, 0]) cube([width - height, height, length]);
        translate([-height/2, -height/2 - 1, 0]) cube([width, height/2 + 1, length]);
    }
}

module rim(showDiffuser = 0)
{
    //union() {
        // outer wall
        difference() {
            union() {
                translate([0, 0, width - wall_thickness * 2])
                    cylinder(r1 = rim_radius - wall_thickness/2, r2 = rim_radius  + wall_thickness/2, h = wall_thickness * 2);
                cylinder(r = rim_radius, h = width - 1);
            }
            translate([0, 0, -1]) cylinder(r = rim_radius - wall_thickness, h = width + 2);
            // bottom cable cutout
            rotate([0, 0, section_angle/2]) translate([0, -rim_radius + wall_thickness -2 , width ]) cable();


            // ribbon cable cutout
            rotate([0, 0, section_angle / 2 - (360/60/2) + 0.5])
                translate([0, rim_radius, wall_thickness + 1])
                     rotate([0, 0, 60])
                        translate([-1, -20, 0]) cube([2, 40, 8]);
        }

        difference() {
            union() {
                // inner wall
                cylinder(r = rim_radius - bar_width  + wall_thickness, h = width);
                // base
                difference() {
                    cylinder(r1 = rim_radius + overhang, r2 = rim_radius + overhang  , h = wall_thickness);
                    translate([0,0,2]) difference(){
                        cylinder(r = rim_radius + overhang + wall_thickness + 2, h = wall_thickness);
                        translate([0, 0, -1]) cylinder(r=rim_radius + 1, h = wall_thickness + 2);
                    }
                }


            }
            // centre cutout
            translate([0, 0, -1])
                cylinder(r = rim_radius - bar_width, h = width + 2);
            // ldr
            rotate([0, 0, -section_angle]) translate([rim_radius - bar_width - 2, 0, width / 2]) ldr();
        }
    //}

    rotate([0, 0, section_angle/2])
        translate([-mount_width/2, rim_radius - bar_width, width - wall_thickness])
            mount();


    if (showDiffuser == 1) {
        rotate([0, 0, - 360/60/2])
        translate([0,0,wall_thickness -1])
            color([0.8,0.8,1.0,0.3]) diffuser();
    }

    for (pn = [ 0 : num_sections - 1 ]) {
        connector(pn);
    }

}

module diffuser()
{
    difference() {
        union() {
            difference() {
                cylinder(r=rim_radius + diffuser_width, h=ribbon_width + wall_thickness + 1);
                translate([0, 0, -1]) cylinder(r=rim_radius + 1, h=ribbon_width + wall_thickness + 2);
                translate([0, 0, 1]) cylinder(r=rim_radius + diffuser_width - 2 * extrusion_width, h=ribbon_width + wall_thickness + 2);
            }

            for(angle = [0:6:359]) {
                rotate([0,0,angle]) translate([- 2 * extrusion_width, rim_radius + 1, 0]) cube([4 * extrusion_width, diffuser_width - 2* extrusion_width,ribbon_width + wall_thickness + 1]);
            }
        }
        // rim top edge
        translate([0, 0, width - 3 * wall_thickness + 1])
            cylinder(r1 = rim_radius - wall_thickness/2, r2 = rim_radius  + wall_thickness, h = wall_thickness * 3);

        // cable
        rotate([0, 0, section_angle/2 + 360/60/2])
        translate([0, - (rim_radius + diffuser_width) , width - 2 ]) cable();

    }
}

module section(sectionNo = 0)
{
    intersection() {
        rim();

        rotate([0, 0, 90 + sectionNo * section_angle]) translate([0, 0, -1])
            wedge(ribbon_width + wall_thickness * 2 + 2, rim_radius + overhang + 30, section_angle);

    }
}

module diff_section(sectionNo = 0)
{
    intersection() {
        diffuser();

        rotate([0, 0, 90 + sectionNo * section_angle]) translate([0, 0, -1])
            wedge(ribbon_width + wall_thickness * 2 + 2, rim_radius + overhang + 30, section_angle);

    }
}


module explode()
{
    for (sn = [ 0 : num_sections - 1 ]) {
        translate([
            -(sin(sn * section_angle + section_angle / 2) * explodeSpace),
            cos(sn * section_angle + section_angle / 2) * explodeSpace, 0])  section(sn);
    }
}

module explode_diff()
{
    for (sn = [ 0 : num_sections - 1 ]) {
        translate([
            -(sin(sn * section_angle + section_angle / 2) * explodeSpace * 2),
            cos(sn * section_angle + section_angle / 2) * explodeSpace * 2, 0])  diff_section(sn);
    }
}

module ribbon()
{
    difference() {
        cylinder(r = rim_radius + 1, h = ribbon_width);
        translate([0, 0, -1]) cylinder(r = rim_radius, h = ribbon_width +2 );
    }

    for (l = [ 0: 59 ]) {
        rotate([0, 0, l * 6]) translate([-2.5, rim_radius, ribbon_width/2 - 2.5]) cube([5, 2, 5]);
    }
}


// these are for show
rim(1);
//explode();
//explode_diff();


// use these for individual parts

// rim section, you only need 0, 1 and 3
//section(0);

// diffuser section, you only need 0 and 3
//diff_section(3);



// =============================================================
//  ESP32 Desk Clock enclosure — "ARCH" version (portrait, arched top)
//  Same parts as clock_case.scad: ESP32 DevKit 30-pin, OLED 0.96",
//  DS3231, FC-51 IR, LDR, DHT11.  Measured module sizes are used.
//
//  Layout (front view, top -> bottom): LDR on the arch, OLED, IR "wave"
//  window, DS3231 on an internal shelf, ESP32 on the floor (USB left side).
//  DHT11 pokes out of the right wall.
//
//  Coordinates: x = left->right (seen from the front), y = front->back,
//  z = up.  Origin = inner front-bottom-left corner.
//  part = "body" | "body_print" | "cover" | "cover_print"
//       | "assembly" | "exploded" | "section"
//
//  Print: body front face down (no supports), cover outside face down.
//  Screws: 4 x M3x10 self-tapping (or heat-set inserts: m3_pilot = 4.0)
// =============================================================
part = "assembly";
cutz = 44;          // section height

$fn = 64;
e = 0.01;

// ---------- box ----------
W  = 60;            // inner width
Hr = 56;            // inner height of the straight part
R  = W / 2;         // arch radius (inner)
H  = Hr + R;        // inner height at the top of the arch
D  = 52;            // inner depth
wall = 2.4;
front_t = 2.0;
cover_t = 2.5;
r_foot = 4;         // bottom outer corner radius
chamfer = 0.8;

// ---------- screw columns: 2 in the bottom corners, 2 inside the arch ----------
col_r = 3.6;
m3_pilot = 2.6;
m3_clear = 3.4;
m3_head_d = 6.6;
arch_a = 35;        // angle of the upper columns above the arch centre line
col_c = [[col_r, col_r], [W - col_r, col_r],
         [R - (R - col_r) * cos(arch_a), Hr + (R - col_r) * sin(arch_a)],
         [R + (R - col_r) * cos(arch_a), Hr + (R - col_r) * sin(arch_a)]];

// ---------- ESP32 DevKit 30-pin (floor, USB to the left wall) ----------
esp_len = 51.5; esp_w = 28.5; esp_t = 1.6;
esp_x0 = 1.2; esp_y0 = 16; esp_z = 14;
esp_grip = 2.2;
esp_cy = esp_y0 + esp_w / 2;
usb_w = 12; usb_h = 8;
usb_cz = esp_z + esp_t + 1.5;

// ---------- DS3231 on a shelf above the ESP32 ----------
ds_w = 22; ds_len = 38;
ds_cx = W / 2;
ds_y0 = 7;
shelf_z = 21; shelf_t = 2;
shelf_top = shelf_z + shelf_t;
ds_top = shelf_top + 10;          // battery holder + PCB + chip

// ---------- IR FC-51 (measured: 14.3 wide, 42.3 incl. LEDs) ----------
ir_w = 14.3; ir_len = 30.8;
ir_cx = W / 2;
ir_led_up = 2.6;
ir_pcb_z = ds_top + 5;
ir_led_reach = 11.5;
ir_y0 = ir_led_reach - front_t;
ir_led_dx = 3.8;
ir_slot_w = 15.5; ir_slot_h = 6.6;
ir_led_cz = ir_pcb_z + ir_led_up;

// ---------- OLED (measured 24.8 x 27.1, pins on top) ----------
oled_w = 24.8; oled_h = 27.1;
oled_x = W / 2 - oled_w / 2;
oled_z = ir_pcb_z + 4 + 4;        // above the IR rails
glass_t = 1.4;
glass_vis_top = 5.5; glass_vis_h = 17;
win_from_top = 13.2;
win_w = 23.5; win_h = 13.5;
glass_cx = W / 2;
glass_cz = oled_z + oled_h - win_from_top;
oled_frame_t = 1.2; oled_gap = 0.3;

// ---------- DHT11 (PCB 16.5 x 21.8) in the right wall ----------
dht_body_w = 15.5; dht_body_h = 12; dht_body_t = 5.5;
dht_pcb_h = 16.5; dht_pcb_len = 21.8;
dht_y0 = 16;
dht_pcb_z0 = 22;
dht_cz = dht_pcb_z0 + dht_body_h / 2;

// ---------- LDR at the top of the arch ----------
ldr_x = W / 2; ldr_y = 10; ldr_d = 5.6;

// ---------- sensor screw holes (positions read from your photos, ±0.5 mm) ----------
// self-tapping screws into plastic: M2 pilot 1.6, M3 pilot 2.5
m2_pilot = 1.6;
m3_pilot_s = 2.5;
// OLED: 4 x M2 at the corners, 1.8 mm from the sides, 1.9 mm from top/bottom
oled_hole_dx = 1.8; oled_hole_dz = 1.9; oled_post_d = 3.6;
// IR: 1 x M3 on the centre line, 7.4 mm from the pin end
ir_hole_from_back = 7.4;
// LDR module (13.9 x 30.2): 1 x M3 on the centre line, 7 mm from the pin end
ldr_pcb_w = 13.9; ldr_pcb_len = 30.2; ldr_hole_from_back = 7;
ldr_pcb_y0 = 12.5;      // front end of the PCB (LDR legs wrap round this end, head up into the hole)
ldr_gap = 3;            // solder side of the PCB faces the arch, this far below it
ldr_pcb_top = Hr + sqrt(R * R - (ldr_pcb_w / 2) * (ldr_pcb_w / 2)) - ldr_gap;
ldr_hole_y = ldr_pcb_y0 + ldr_pcb_len - ldr_hole_from_back;
// DHT11: 1 x M2 beside the sensor body, 5 mm from the sensor end, 14.7 mm from the body-side edge
dht_hole_y = 5; dht_hole_z = 14.7;

// ---------- decoration ----------
groove_in = 3.5;  groove_w = 0.8;  groove_d = 0.5;

// =============================================================
//  helpers
// =============================================================
module along_y(y0, len) translate([0, y0 + len, 0]) rotate([90, 0, 0]) linear_extrude(len) children();
module rrect(w, h, r) translate([r, r]) offset(r) square([w - 2 * r, h - 2 * r]);
module rrect_c(w, h, r) translate([-w / 2, -h / 2]) rrect(w, h, r);

// inner cavity profile in the x-z plane: rectangle + half circle
module inner2d() union() {
  square([W, Hr]);
  translate([R, Hr]) circle(R, $fn = 128);
}

// outer profile; inset > 0 shrinks it (for chamfers)
module outer2d(inset = 0)
  offset(r = r_foot) offset(delta = -r_foot) offset(delta = wall - inset) inner2d();

module columns2d() for (i = [0 : 3]) {
  c = col_c[i];
  if (i < 2) hull() {
    translate(c) circle(col_r);
    translate([c[0] < W / 2 ? 0 : W - col_r, 0]) square(col_r);
  } else hull() {
    translate(c) circle(col_r);
    // tie into the arch wall
    a = (i == 2) ? 180 - arch_a : arch_a;
    translate([R + (R + 1) * cos(a), Hr + (R + 1) * sin(a)]) circle(1);
  }
}

module rbox_x(x0, len, yc, zc, wy, hz, r) hull()
  for (yy = [yc - wy / 2 + r, yc + wy / 2 - r], zz = [zc - hz / 2 + r, zc + hz / 2 - r])
    translate([x0, yy, zz]) rotate([0, 90, 0]) cylinder(r = r, h = len);

module front_hole(cx, cz, w, h, r) {
  along_y(-front_t - 1, front_t + 2) translate([cx, cz]) rrect_c(w, h, r);
  hull() {
    along_y(-front_t - e, e) translate([cx, cz]) rrect_c(w + 1.6, h + 1.6, r + 0.8);
    along_y(-front_t + 0.8, e) translate([cx, cz]) rrect_c(w, h, r);
  }
}

// =============================================================
//  body
// =============================================================
module shell() difference() {
  hull() {
    along_y(-front_t, e) outer2d(chamfer);
    along_y(-front_t + chamfer, D + front_t - chamfer) outer2d(0);
  }
  along_y(0, D + 1) inner2d();
}

module oled_frame() along_y(0, glass_t + 1.2 + 1.0) difference() {
  translate([oled_x - oled_gap - oled_frame_t, oled_z - oled_gap - oled_frame_t])
    square([oled_w + 2 * (oled_gap + oled_frame_t), oled_h + 2 * (oled_gap + oled_frame_t)]);
  translate([oled_x - oled_gap, oled_z - oled_gap]) square([oled_w + 2 * oled_gap, oled_h + 2 * oled_gap]);
  translate([oled_x + 4, oled_z + oled_h - 1]) square([oled_w - 8, 5]);   // wire exit
}

module esp_cradles() {
  xe = esp_x0 + esp_len;
  difference() {
    translate([xe - esp_grip, 0, 0]) cube([esp_grip + 3.5, D - 2.5, esp_z + esp_t + 0.4 + 2.2]);
    translate([xe - esp_grip - e, esp_y0, esp_z]) cube([esp_grip + 0.4 + e, D, esp_t + 0.4]);
  }
  translate([-e, esp_cy - 8, 0]) cube([esp_x0 + 2.0, 16, esp_z]);
}

// shelf for the DS3231, grown from the front wall (prints as an upright fin)
module ds_shelf() {
  sx0 = ds_cx - ds_w / 2 - 2.5;
  translate([sx0, 0, shelf_z]) cube([ds_w + 5, ds_y0 + ds_len + 1, shelf_t]);
  // side rails
  for (x = [ds_cx - ds_w / 2 - 1.7, ds_cx + ds_w / 2 + 0.5])
    translate([x, ds_y0, shelf_top - e]) cube([1.2, ds_len, 3]);
  // stiffening rib under the shelf, down the middle (clear of the ESP32 module)
  translate([ds_cx - 0.8, 0, esp_z + esp_t + 3.6]) cube([1.6, 12, shelf_z - (esp_z + esp_t + 3.6) + e]);
}

module ir_rails() {
  gh = ir_w / 2 + 0.3;
  hy = ir_y0 + ir_len - ir_hole_from_back;
  for (s = [-1, 1]) {
    translate([ir_cx + (s < 0 ? -gh - 2.5 : gh), 0, ir_pcb_z - 4]) cube([2.5, ir_y0 + ir_len - 2, 8]);
    translate([ir_cx + (s < 0 ? -gh : gh - 1.5), ir_y0 - 1, ir_pcb_z - 4]) cube([1.5, ir_len - 1, 4]);
  }
  // cross plate + screw boss under the PCB hole (the screw now sets the position)
  translate([ir_cx - gh - 2.5, hy - 3.5, ir_pcb_z - 4]) cube([2 * (gh + 2.5), 7, 2.5]);
  translate([ir_cx, hy, ir_pcb_z - 4]) cylinder(d = 6, h = 4);
}

module oled_posts()
  for (hx = [oled_hole_dx, oled_w - oled_hole_dx], hz = [oled_hole_dz, oled_h - oled_hole_dz])
    translate([oled_x + hx, -e, oled_z + hz]) rotate([-90, 0, 0]) cylinder(d = oled_post_d, h = glass_t + e);

// boss hanging from the arch for the LDR module, with a 45 deg gusset so it prints
module ldr_boss() {
  drop = H - ldr_pcb_top;
  hull() {
    translate([ldr_x, ldr_hole_y, ldr_pcb_top]) cylinder(d = 6, h = drop + 1.5);
    translate([ldr_x - 3, ldr_hole_y - drop - 3, H - 0.6]) cube([6, 0.1, 2]);
  }
}

module sensor_pilots() {
  // OLED M2 (blind, leaves 0.6 mm of front wall)
  for (hx = [oled_hole_dx, oled_w - oled_hole_dx], hz = [oled_hole_dz, oled_h - oled_hole_dz])
    translate([oled_x + hx, -front_t + 0.6, oled_z + hz]) rotate([-90, 0, 0]) cylinder(d = m2_pilot, h = front_t - 0.6 + glass_t + e);
  // IR M3
  translate([ir_cx, ir_y0 + ir_len - ir_hole_from_back, ir_pcb_z - 4 - e]) cylinder(d = m3_pilot_s, h = 4 + 2 * e);
  // LDR M3 (blind, from below)
  translate([ldr_x, ldr_hole_y, ldr_pcb_top - e]) cylinder(d = m3_pilot_s, h = H - ldr_pcb_top + 1);
  // DHT11 M2 into the right wall (blind, 2 mm deep)
  translate([W - e, dht_y0 + dht_hole_y, dht_pcb_z0 + dht_hole_z]) rotate([0, 90, 0]) cylinder(d = m2_pilot, h = 2 + e);
}

module dht_guides()
  for (z = [dht_pcb_z0 - 0.3 - 2, dht_pcb_z0 + dht_pcb_h + 0.3])
    translate([W - 4, dht_y0 - 2, z]) cube([4 + e, dht_pcb_len + 2, 2]);

module body() difference() {
  union() {
    shell();
    intersection() { along_y(0, D) columns2d(); along_y(-1, D + 2) offset(delta = 0.5) inner2d(); }
    oled_frame();
    esp_cradles();
    ds_shelf();
    ir_rails();
    dht_guides();
    oled_posts();
    ldr_boss();
  }
  sensor_pilots();
  front_hole(glass_cx, glass_cz, win_w, win_h, 1.2);
  front_hole(ir_cx, ir_led_cz, ir_slot_w, ir_slot_h, ir_slot_h / 2 - 0.01);
  translate([ldr_x, ldr_y, H - 2]) cylinder(d = ldr_d, h = wall + 4);
  rbox_x(W - 1, wall + 2, dht_y0 + dht_body_w / 2, dht_cz, dht_body_w + 0.8, dht_body_h + 0.8, 1);
  rbox_x(-wall - 1, wall + 2, esp_cy, usb_cz, usb_w, usb_h, 1.5);
  for (x = [10 : 5 : 45]) translate([x, esp_y0 + 3, -wall - 1]) cube([2, esp_w - 6, wall + 2]);
  for (c = col_c) translate([c[0], D - 14, c[1]]) rotate([-90, 0, 0]) cylinder(d = m3_pilot, h = 14 + e);
  // decorative groove that follows the outline of the face
  along_y(-front_t - e, groove_d + e) difference() {
    offset(delta = -groove_in) outer2d(0);
    offset(delta = -groove_in - groove_w) outer2d(0);
  }
}

// =============================================================
//  back cover
// =============================================================
lip_d = 2; lip_w = 1.6; lip_gap = 0.3;

module cover() difference() {
  union() {
    hull() {
      along_y(D, cover_t - chamfer) outer2d(0);
      along_y(D + cover_t - e, e) outer2d(chamfer);
    }
    along_y(D - lip_d, lip_d + e) difference() {
      offset(delta = -lip_gap) inner2d();
      offset(delta = -lip_gap - lip_w) inner2d();
      offset(0.4) columns2d();
    }
  }
  for (c = col_c) translate([c[0], 0, c[1]]) {
    translate([0, D - 1, 0]) rotate([-90, 0, 0]) cylinder(d = m3_clear, h = cover_t + 2);
    translate([0, D + cover_t - 1.7, 0]) rotate([-90, 0, 0]) cylinder(d1 = m3_clear, d2 = m3_head_d, h = 1.7 + e);
  }
  // vents: low for the ESP32, high in the arch for warm air
  for (x = [12 : 5 : 46]) translate([x, D - 1, 8]) cube([2, cover_t + 2, 12]);
  for (x = [20 : 5 : 40]) translate([x, D - 1, 64]) cube([2, cover_t + 2, 12]);
  along_y(D + cover_t - 0.6, 1) translate([W / 2, 40]) mirror([1, 0])
    text("ESP32 CLOCK", size = 4.2, halign = "center", font = "Liberation Sans:style=Bold");
}

// =============================================================
//  stand-in modules (fit check only)
// =============================================================
module phantoms() {
  // OLED
  color("royalblue") translate([oled_x, glass_t, oled_z]) cube([oled_w, 1.2, oled_h]);
  color("black") translate([oled_x + 0.3, 0, oled_z + oled_h - glass_vis_top - glass_vis_h]) cube([oled_w - 0.6, glass_t, glass_vis_h]);
  color("gold") translate([oled_x + 7.3, 2.6, oled_z + oled_h - 2.5]) cube([10.2, 6, 1]);
  // ESP32
  color("#202020") translate([esp_x0, esp_y0, esp_z]) cube([esp_len, esp_w, esp_t]);
  color("silver") translate([esp_x0 + esp_len - 25.5, esp_cy - 9, esp_z + esp_t]) cube([25.5, 18, 3.2]);
  color("silver") translate([esp_x0 - 0.6, esp_cy - 3.9, esp_z + esp_t]) cube([5.6, 7.8, 2.8]);
  color("gold") for (y = [esp_y0 + 0.75, esp_y0 + esp_w - 3.25])
    translate([esp_x0 + 4, y, esp_z - 11.5]) cube([38.1, 2.5, 11.5]);
  // DS3231
  color("royalblue") translate([ds_cx - ds_w / 2, ds_y0, shelf_top + 5]) cube([ds_w, ds_len, 1.6]);
  color("#222") translate([ds_cx, ds_y0 + 14, shelf_top]) cylinder(d = 21, h = 5);
  color("#333") translate([ds_cx - 4, ds_y0 + 12, shelf_top + 6.6]) cube([8, 10, 3]);
  color("gold") for (y = [ds_y0 - 6, ds_y0 + ds_len]) translate([ds_cx - 7, y, shelf_top + 6]) cube([14, 6, 1]);
  // IR
  color("royalblue") translate([ir_cx - ir_w / 2, ir_y0, ir_pcb_z]) cube([ir_w, ir_len, 1.6]);
  color("white") translate([ir_cx - ir_led_dx, ir_y0, ir_led_cz]) rotate([90, 0, 0]) cylinder(d = 5, h = ir_led_reach);
  color("#111") translate([ir_cx + ir_led_dx, ir_y0, ir_led_cz]) rotate([90, 0, 0]) cylinder(d = 5, h = ir_led_reach);
  color("dodgerblue") translate([ir_cx - 3.5, ir_y0 + 12, ir_pcb_z + 1.6]) cube([7, 7, 5]);
  // DHT11
  color("#202020") translate([W - 1.6, dht_y0, dht_pcb_z0]) cube([1.6, dht_pcb_len, dht_pcb_h]);
  color("lightskyblue") translate([W, dht_y0, dht_pcb_z0]) cube([dht_body_t, dht_body_w, dht_body_h]);
  // LDR module under the arch, solder side up, components hanging down
  color("royalblue") translate([ldr_x - ldr_pcb_w / 2, ldr_pcb_y0, ldr_pcb_top - 1.6]) cube([ldr_pcb_w, ldr_pcb_len, 1.6]);
  color("dodgerblue") translate([ldr_x - ldr_pcb_w / 2 + 0.5, ldr_pcb_y0 + 10, ldr_pcb_top - 1.6 - 6]) cube([6.5, 7, 6]);
  color("silver") translate([ldr_x, ldr_hole_y, ldr_pcb_top - 1.6 - 2]) cylinder(d = 5.5, h = 2);
  color("orange") translate([ldr_x, ldr_y, ldr_pcb_top - 1]) cylinder(d = 5, h = H - ldr_pcb_top + 1 + wall - 0.3);
  // screw heads
  color("silver") translate([ir_cx, ir_y0 + ir_len - ir_hole_from_back, ir_pcb_z + 1.6]) cylinder(d = 5.5, h = 2);
}

// =============================================================
if (part == "body") body();
else if (part == "body_print") translate([0, 0, front_t]) rotate([90, 0, 0]) body();
else if (part == "cover") cover();
else if (part == "cover_print") translate([0, 0, D + cover_t]) rotate([-90, 0, 0]) cover();
else if (part == "assembly") { color("#2f3a4a") body(); color("#26303d") cover(); phantoms(); }
else if (part == "exploded") { color("#2f3a4a") body(); translate([0, 45, 0]) color("#26303d") cover(); phantoms(); }
else if (part == "section") {
  difference() { union() { color("#2f3a4a") body(); color("#26303d") cover(); }
                 translate([-10, -10, -10]) cube([W + 20, 20 + 10, H + 20]); }
  phantoms();
}

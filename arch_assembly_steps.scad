// Assembly-step renders for clock_case_arch.scad
// step = 1..7 : parts before `step` are installed, part `step` floats at its insertion offset with a red arrow
include <clock_case_arch.scad>
part = "none";
step = 1;

// ---------- one module per part ----------
module p_oled() {
  color("royalblue") translate([oled_x, glass_t, oled_z]) cube([oled_w, 1.2, oled_h]);
  color("black") translate([oled_x + 0.3, 0, oled_z + oled_h - glass_vis_top - glass_vis_h]) cube([oled_w - 0.6, glass_t, glass_vis_h]);
  color("gold") translate([oled_x + 7.3, 2.6, oled_z + oled_h - 2.5]) cube([10.2, 6, 1]);
  color("silver") for (hx = [oled_hole_dx, oled_w - oled_hole_dx], hz = [oled_hole_dz, oled_h - oled_hole_dz])
    translate([oled_x + hx, glass_t + 1.2, oled_z + hz]) rotate([-90, 0, 0]) cylinder(d = 3.6, h = 1.4);
}
module p_ds() {
  color("royalblue") translate([ds_cx - ds_w / 2, ds_y0, shelf_top + 5]) cube([ds_w, ds_len, 1.6]);
  color("#222") translate([ds_cx, ds_y0 + 14, shelf_top]) cylinder(d = 21, h = 5);
  color("#333") translate([ds_cx - 4, ds_y0 + 12, shelf_top + 6.6]) cube([8, 10, 3]);
  color("gold") for (y = [ds_y0 - 6, ds_y0 + ds_len]) translate([ds_cx - 7, y, shelf_top + 6]) cube([14, 6, 1]);
}
module p_ir() {
  color("royalblue") translate([ir_cx - ir_w / 2, ir_y0, ir_pcb_z]) cube([ir_w, ir_len, 1.6]);
  color("white") translate([ir_cx - ir_led_dx, ir_y0, ir_led_cz]) rotate([90, 0, 0]) cylinder(d = 5, h = ir_led_reach);
  color("#111") translate([ir_cx + ir_led_dx, ir_y0, ir_led_cz]) rotate([90, 0, 0]) cylinder(d = 5, h = ir_led_reach);
  color("dodgerblue") translate([ir_cx - 6, ir_y0 + 12, ir_pcb_z + 1.6]) cube([6, 6, 5]);
  color("silver") translate([ir_cx, ir_y0 + ir_len - ir_hole_from_back, ir_pcb_z + 1.6]) cylinder(d = 5.5, h = 2);
}
module p_ldr() {
  color("royalblue") translate([ldr_x - ldr_pcb_w / 2, ldr_pcb_y0, ldr_pcb_top - 1.6]) cube([ldr_pcb_w, ldr_pcb_len, 1.6]);
  color("dodgerblue") translate([ldr_x - ldr_pcb_w / 2 + 0.5, ldr_pcb_y0 + 10, ldr_pcb_top - 1.6 - 6]) cube([6.5, 7, 6]);
  color("silver") translate([ldr_x, ldr_hole_y, ldr_pcb_top - 1.6 - 2]) cylinder(d = 5.5, h = 2);
  color("orange") translate([ldr_x, ldr_y, ldr_pcb_top - 1]) cylinder(d = 5, h = H - ldr_pcb_top + 1 + wall - 0.3);
  color("silver") translate([ldr_x - 1, ldr_y, ldr_pcb_top - 1]) cube([0.6, ldr_pcb_y0 - ldr_y + 1, 0.6]);
}
module p_dht() {
  color("#202020") translate([W - 1.6, dht_y0, dht_pcb_z0]) cube([1.6, dht_pcb_len, dht_pcb_h]);
  color("lightskyblue") translate([W, dht_y0, dht_pcb_z0]) cube([dht_body_t, dht_body_w, dht_body_h]);
  color("silver") translate([W - 1.6, dht_y0 + dht_hole_y, dht_pcb_z0 + dht_hole_z]) rotate([0, -90, 0]) cylinder(d = 3.8, h = 1.4);
}
module p_esp() {
  color("#202020") translate([esp_x0, esp_y0, esp_z]) cube([esp_len, esp_w, esp_t]);
  color("silver") translate([esp_x0 + esp_len - 25.5, esp_cy - 9, esp_z + esp_t]) cube([25.5, 18, 3.2]);
  color("silver") translate([esp_x0 - 0.6, esp_cy - 3.9, esp_z + esp_t]) cube([5.6, 7.8, 2.8]);
  color("gold") for (y = [esp_y0 + 0.75, esp_y0 + esp_w - 3.25])
    translate([esp_x0 + 4, y, esp_z - 11.5]) cube([38.1, 2.5, 11.5]);
}
module p_cover() color("#26303d") cover();

// offset each part floats at before it goes in, and the arrow anchor (its final centre)
offs = [[0, 0, 0],
        [0, 70, 0],      // 1 OLED
        [0, 75, 0],      // 2 DS3231
        [0, 75, 0],      // 3 IR
        [0, 72, -4],     // 4 LDR
        [-18, 60, 0],    // 5 DHT11
        [0, 75, 0],      // 6 ESP32
        [0, 45, 0]];     // 7 cover
anch = [[0, 0, 0],
        [W / 2, 4, oled_z + oled_h / 2],
        [ds_cx, ds_y0 + ds_len / 2, shelf_top + 8],
        [ir_cx, ir_y0 + ir_len / 2, ir_pcb_z + 6],
        [ldr_x, ldr_pcb_y0 + ldr_pcb_len / 2, ldr_pcb_top - 9],
        [W - 4, dht_y0 + dht_pcb_len / 2, dht_pcb_z0 + dht_pcb_h + 4],
        [esp_x0 + esp_len / 2, esp_y0 + esp_w / 2, esp_z + 7],
        [W / 2, D + 2, Hr]];

module part_n(n) {
  if (n == 1) p_oled();
  else if (n == 2) p_ds();
  else if (n == 3) p_ir();
  else if (n == 4) p_ldr();
  else if (n == 5) p_dht();
  else if (n == 6) p_esp();
  else if (n == 7) p_cover();
}

// red arrow from the floating part to its final place
module arrow(n) {
  o = offs[n];
  L = norm(o);
  a = anch[n];
  dir = o / L;
  color("red") translate(a) {
    // rotate +z onto dir
    b = acos(dir[2]);
    c = atan2(dir[1], dir[0]);
    rotate([0, b, c]) {
      cylinder(r1 = 0, r2 = 2.6, h = 6);
      translate([0, 0, 6]) cylinder(r = 0.9, h = L - 6);
    }
  }
}

color("#8a94a2", 0.33) body();
for (n = [1 : 7]) if (n < step) part_n(n);
if (step >= 1 && step <= 7) { translate(offs[step]) part_n(step); arrow(step); }

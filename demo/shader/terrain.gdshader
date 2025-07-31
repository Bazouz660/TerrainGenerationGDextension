shader_type spatial;

group_uniforms textures;
uniform sampler2D top_texture: source_color;
uniform sampler2D top_normal: hint_normal;
uniform sampler2D top_roughness: hint_roughness_r;
uniform sampler2D bottom_texture: source_color;
uniform sampler2D bottom_normal: hint_normal;
uniform sampler2D bottom_roughness: hint_roughness_r;
uniform sampler2D beach_texture: source_color;
uniform sampler2D beach_normal: hint_normal;
uniform sampler2D beach_roughness: hint_roughness_r;

group_uniforms slopes;
uniform float slope_threshold: hint_range(0.0, 1.0, 0.01) = 0.5;
uniform float slope_transition: hint_range(0.0, 1.0, 0.01) = 0.1;
uniform bool use_world_normal = true;

group_uniforms height_blending;
uniform float water_level: hint_range(-100.0, 100.0, 0.1) = 0.0;
uniform float beach_transition: hint_range(0.0, 50.0, 0.1) = 2.0;

varying float world_normal_y;
varying float normal_y;
varying float world_position_y;

group_uniforms uv;
varying vec3 uv_top_triplanar_pos;
uniform float uv_top_blend_sharpness = 1.0;
varying vec3 uv_top_power_normal;
uniform vec3 uv_top_scale = vec3(1.0, 1.0, 1.0);
uniform vec3 uv_top_offset = vec3(0.0, 0.0, 0.0);

varying vec3 uv_bottom_triplanar_pos;
uniform float uv_bottom_blend_sharpness = 1.0;
varying vec3 uv_bottom_power_normal;
uniform vec3 uv_bottom_scale = vec3(1.0, 1.0, 1.0);
uniform vec3 uv_bottom_offset = vec3(0.0, 0.0, 0.0);

varying vec3 uv_beach_triplanar_pos;
uniform float uv_beach_blend_sharpness = 1.0;
varying vec3 uv_beach_power_normal;
uniform vec3 uv_beach_scale = vec3(1.0, 1.0, 1.0);
uniform vec3 uv_beach_offset = vec3(0.0, 0.0, 0.0);


void vertex() {
	normal_y = NORMAL.y;
	vec3 world_normal = normalize((MODEL_MATRIX * vec4(NORMAL, 0.0)).xyz);
	world_normal_y = world_normal.y;

	// Calculate world position Y for height-based blending
	vec4 world_position = MODEL_MATRIX * vec4(VERTEX, 1.0);
	world_position_y = world_position.y;

	vec3 normal = NORMAL;
	TANGENT = vec3(0.0,0.0,-1.0) * abs(normal.x);
	TANGENT+= vec3(1.0,0.0,0.0) * abs(normal.y);
	TANGENT+= vec3(1.0,0.0,0.0) * abs(normal.z);
	TANGENT = normalize(TANGENT);
	BINORMAL = vec3(0.0,1.0,0.0) * abs(normal.x);
	BINORMAL+= vec3(0.0,0.0,-1.0) * abs(normal.y);
	BINORMAL+= vec3(0.0,1.0,0.0) * abs(normal.z);
	BINORMAL = normalize(BINORMAL);

	uv_top_power_normal=pow(abs(NORMAL),vec3(uv_top_blend_sharpness));
	uv_top_triplanar_pos = VERTEX * uv_top_scale + uv_top_offset;
	uv_top_power_normal/=dot(uv_top_power_normal,vec3(1.0));
	uv_top_triplanar_pos *= vec3(1.0,-1.0, 1.0);

	uv_bottom_power_normal=pow(abs(NORMAL),vec3(uv_bottom_blend_sharpness));
	uv_bottom_triplanar_pos = VERTEX * uv_bottom_scale + uv_bottom_offset;
	uv_bottom_power_normal/=dot(uv_bottom_power_normal,vec3(1.0));
	uv_bottom_triplanar_pos *= vec3(1.0,-1.0, 1.0);

	uv_beach_power_normal=pow(abs(NORMAL),vec3(uv_beach_blend_sharpness));
	uv_beach_triplanar_pos = VERTEX * uv_beach_scale + uv_beach_offset;
	uv_beach_power_normal/=dot(uv_beach_power_normal,vec3(1.0));
	uv_beach_triplanar_pos *= vec3(1.0,-1.0, 1.0);
}

vec2 hash( vec2 p ) {
	return fract( sin( p * mat2( vec2( 127.1, 311.7 ), vec2( 269.5, 183.3 ) ) ) * 43758.5453 );
}

vec4 stochastic_sample(sampler2D tex, vec2 uv) {
	vec2 skewV = mat2(vec2(1.0,1.0),vec2(-0.57735027 , 1.15470054))*uv * 3.464;

	vec2 vxID = floor(skewV);
	vec2 fracV = fract(skewV);
	vec3 barry = vec3(fracV.x,fracV.y,1.0-fracV.x-fracV.y);

	mat4 bw_vx = barry.z>0.0?
		mat4(vec4(vxID,0.0,0.0),vec4((vxID+vec2(0.0,1.0)),0.0,0.0),vec4(vxID+vec2(1.0,0.0),0,0),vec4(barry.zyx,0)):
		mat4(vec4(vxID+vec2(1.0,1.0),0.0,0.0),vec4((vxID+vec2(1.0,0.0)),0.0,0.0),vec4(vxID+vec2(0.0,1.0),0,0),vec4(-barry.z,1.0-barry.y,1.0-barry.x,0));

	vec2 ddx = dFdx(uv);
	vec2 ddy = dFdy(uv);

	return (textureGrad(tex,uv+hash(bw_vx[0].xy),ddx,ddy)*bw_vx[3].x) +
	(textureGrad(tex,uv+hash(bw_vx[1].xy),ddx,ddy)*bw_vx[3].y) +
	(textureGrad(tex,uv+hash(bw_vx[2].xy),ddx,ddy)*bw_vx[3].z);
}

vec4 triplanar_stochastic_texture(sampler2D p_sampler,vec3 p_weights,vec3 p_triplanar_pos) {
	vec4 samp=vec4(0.0);
	samp+= stochastic_sample(p_sampler,p_triplanar_pos.xy) * p_weights.z;
	samp+= stochastic_sample(p_sampler,p_triplanar_pos.xz) * p_weights.y;
	samp+= stochastic_sample(p_sampler,p_triplanar_pos.zy * vec2(-1.0,1.0)) * p_weights.x;
	return samp;
}

void fragment() {
	// Albedo values
	vec3 top_albedo = triplanar_stochastic_texture(top_texture,uv_top_power_normal,uv_top_triplanar_pos).xyz;
	vec3 bottom_albedo = triplanar_stochastic_texture(bottom_texture,uv_bottom_power_normal,uv_bottom_triplanar_pos).xyz;
	vec3 beach_albedo = triplanar_stochastic_texture(beach_texture,uv_beach_power_normal,uv_beach_triplanar_pos).xyz;

	// Normal values
	vec3 top_normal_map = triplanar_stochastic_texture(top_normal,uv_top_power_normal,uv_top_triplanar_pos).xyz;
	vec3 bottom_normal_map = triplanar_stochastic_texture(bottom_normal,uv_bottom_power_normal,uv_bottom_triplanar_pos).xyz;
	vec3 beach_normal_map = triplanar_stochastic_texture(beach_normal,uv_beach_power_normal,uv_beach_triplanar_pos).xyz;

	// Roughness values
	vec4 top_roughness_map = triplanar_stochastic_texture(top_roughness,uv_top_power_normal,uv_top_triplanar_pos);
	vec4 bottom_roughness_map = triplanar_stochastic_texture(bottom_roughness,uv_bottom_power_normal,uv_bottom_triplanar_pos);
	vec4 beach_roughness_map = triplanar_stochastic_texture(beach_roughness,uv_beach_power_normal,uv_beach_triplanar_pos);

	// Calculate slope factor
	float slope_factor = world_normal_y;

	if (!use_world_normal) {
		slope_factor = normal_y;
	}

	// Calculate blend weight based on slope
	// When slope_factor is close to 1.0 (flat), use top texture
	// When slope_factor is close to 0.0 or negative (steep), use bottom texture
	float slope_blend_weight = smoothstep(slope_threshold - slope_transition, slope_threshold + slope_transition, slope_factor);

	// Calculate height-based blend weight for beach texture
	// When below water_level, use beach texture
	// When above water_level + beach_transition, use slope-based blend
	float height_blend_weight = smoothstep(water_level, water_level + beach_transition, world_position_y);

	// First blend top and bottom textures based on slope
	vec3 slope_blended_albedo = mix(bottom_albedo, top_albedo, slope_blend_weight);
	vec3 slope_blended_normal = mix(bottom_normal_map, top_normal_map, slope_blend_weight);
	vec4 slope_blended_roughness = mix(bottom_roughness_map, top_roughness_map, slope_blend_weight);

	// Beach texture should only affect areas that would use the top texture
	// Mix beach with top texture first, then apply slope blending
	vec3 beach_top_albedo = mix(beach_albedo, top_albedo, height_blend_weight);
	vec3 beach_top_normal = mix(beach_normal_map, top_normal_map, height_blend_weight);
	vec4 beach_top_roughness = mix(beach_roughness_map, top_roughness_map, height_blend_weight);

	// Apply slope blending between bottom texture and beach-affected top texture
	vec4 roughness_texture_channel = vec4(1.0,0.0,0.0,0.0);

	ALBEDO = mix(bottom_albedo, beach_top_albedo, slope_blend_weight);
	NORMAL_MAP = mix(bottom_normal_map, beach_top_normal, slope_blend_weight);
	ROUGHNESS = dot(mix(bottom_roughness_map, beach_top_roughness, slope_blend_weight), roughness_texture_channel);
}
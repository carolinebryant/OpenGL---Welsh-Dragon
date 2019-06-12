#define BLEND_REPLACE 1
#define BLEND_NONE 2

#define M_PI 3.141592653589793238462643383279

varying vec3 ec_vnormal, ec_vposition;
varying vec3 wc_vnormal;

uniform int blend_mode;
uniform sampler2D mytexture;
uniform bool[6] lightsEnabled;

void main()
{
    int lightSourceCount = 6;

    vec3 P, N, V, D;
    vec4 tcolor;
    vec4 diffuse_m = gl_FrontMaterial.diffuse;
    vec4 specular_m = gl_FrontMaterial.specular;
    vec4 diffuse_color, specular_color;
    float shininess = gl_FrontMaterial.shininess;
    P = ec_vposition;
    N = normalize(ec_vnormal);
    V = normalize(-P);

    gl_FragColor = vec4(0,0,0,0);

	int lightI;
	for (lightI = 0; lightI < lightSourceCount; lightI++)
	{
        if (lightsEnabled[lightI]) {
            vec3 L = normalize(gl_LightSource[lightI].position.xyz - P);
    		vec3 H = normalize(L+V);

            tcolor = texture2D(mytexture, gl_TexCoord[0].st);
            if (blend_mode == BLEND_NONE){
                diffuse_color = gl_LightSource[lightI].diffuse * max(dot(N,L),0.0) * diffuse_m * gl_LightSource[lightI].diffuse.w;
                specular_color = gl_LightSource[lightI].specular * (((shininess + 2.0) / 8.0 * M_PI) * pow(max(dot(H,N),0.0),shininess)) * specular_m * gl_LightSource[lightI].specular.w;
            }
            else if (blend_mode == BLEND_REPLACE) {
                diffuse_color = gl_LightSource[lightI].diffuse * max(dot(N,L),0.0) * tcolor * gl_LightSource[lightI].diffuse.w;
                specular_color = gl_LightSource[lightI].specular * (((shininess + 2.0) / 8.0 * M_PI) * pow(max(dot(H,N),0.0),shininess)) * tcolor * gl_LightSource[lightI].specular.w;
            }
            else {
                diffuse_color = vec4(1, 0, 0, 1);
                specular_color = vec4(0.5, 0.5, 0.5, 1);
            }
            gl_FragColor += diffuse_color + specular_color;
        }
	}
    gl_FragColor = vec4(gl_FragColor.xyz, diffuse_m.w);
}

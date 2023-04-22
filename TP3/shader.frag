// --------------------------------------------------------------------------
// gMini,
// a minimal Glut/OpenGL app to extend                              
//
// Copyright(C) 2007-2009                
// Tamy Boubekeur
//                                                                            
// All rights reserved.                                                       
//                                                                            
// This program is free software; you can redistribute it and/or modify       
// it under the terms of the GNU General Public License as published by       
// the Free Software Foundation; either version 2 of the License, or          
// (at your option) any later version.                                        
//                                                                            
// This program is distributed in the hope that it will be useful,            
// but WITHOUT ANY WARRANTY; without even the implied warranty of             
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              
// GNU General Public License (http://www.gnu.org/licenses/gpl.txt)           
// for more details.                                                          
//                                                                          
// --------------------------------------------------------------------------

uniform float ambientRef;
uniform float diffuseRef;
uniform float specularRef;
uniform float shininess;

varying vec4 p;
varying vec3 n;

void main (void) {
    vec3 P = vec3 (gl_ModelViewMatrix * p);     //Position du point à éclairer
    vec3 N = normalize (gl_NormalMatrix * n);   //Normal en ce point
    vec3 V = normalize (-P);                    //Vecteur de vue
    
    vec4 Ka = gl_FrontMaterial.ambient;
    vec4 Kd = gl_FrontMaterial.diffuse;
    vec4 Ks = gl_FrontMaterial.specular;

    vec4 Isa = gl_LightSource[0].ambient;           //Objet openGL qui contien la lumière ambiente
    vec4 Ia = Isa * Ka;
    vec4 I = ambientRef * Ia;

    for (int i = 0; i < 4 ; i++) {  //1 et pas 3 pour la 2. (Il nous faut 1 lumière)
        if (i != 0) {
            Isa = gl_LightSource[i].ambient;
            Ia = Isa * Ka;
            I += ambientRef * Ia;
        }
        vec4 Isd = gl_LightSource[i].diffuse;
        vec4 Iss = gl_LightSource[i].specular;
        
        vec3 L = normalize(n - gl_LightSource[i].position.xyz);     //Normalisation du point d'arrivé - départ

        float dln = dot(N, L);
        if (0.75 <= dln && dln <= 1.) {
            dln = 0.75;
        }
        if (0.5 <= dln && dln < 0.75) {
            dln = 0.5;
        }
        if (0.25 <= dln && dln < 0.50) {
            dln = 0.25;
        }
        if (0. <= dln && dln < 0.25){
            dln = 0.;
        }
    
        vec3 R = normalize(2.f * (dot(N, L)) * N - L);              //Cf cours Diapo 43

        vec4 Id = Isd * Kd * dln;   //dln = dot(L, N)               //Cf cours
        vec4 Is = Iss * Ks * pow(dot(R, V), shininess);             //Cf cours

        I += ambientRef * Ia;
        I += Id * diffuseRef;
        I += Is * specularRef;
    }

    ////////////////////////////////////////////////
    //Eclairage de Phong à calculer en utilisant
    ///////////////////////////////////////////////
    // gl_LightSource[i].position.xyz Position de la lumière i
    // gl_LightSource[i].diffuse Couleur diffuse de la lumière i
    // gl_LightSource[i].specular Couleur speculaire de la lumière i
    // gl_FrontMaterial.diffuse Matériaux diffus de l'objet
    // gl_FrontMaterial.specular Matériaux speculaire de l'objet


    gl_FragColor = vec4 (I.xyz, 1);
}


#include "types.h"


struct RayIntersection
{
	bool hit;
	float32 fraction;
	vec3f point;
	vec3f normal;
};

const Box COLLIDER_BOX_TABLE[20]
{
	//Full Inner Pieces
	Box(vec3f(   0.0f, 0.0f, 0.0f),vec3f( 1.0f, 1.0f, 1.0f)),
	Box(vec3f(   0.0f, 0.0f, 0.0f),vec3f( 1.0f, 1.0f, 1.0f)),
	Box(vec3f(   0.0f, 0.0f, 0.0f),vec3f( 1.0f, 1.0f, 1.0f)),
	Box(vec3f(   0.0f, 0.0f, 0.0f),vec3f( 1.0f, 1.0f, 1.0f)),

	//Side Straight Pieces
	Box(vec3f(-CELL_QUAR, 0.0f, 0.0f),vec3f(CELL_HALF,CELL_SIZE,CELL_SIZE)),
	Box(vec3f( 0.0f, 0.0f, CELL_QUAR),vec3f(CELL_SIZE,CELL_SIZE,CELL_HALF)),
	Box(vec3f( CELL_QUAR, 0.0f, 0.0f),vec3f(CELL_HALF,CELL_SIZE,CELL_SIZE)),
	Box(vec3f( 0.0f, 0.0f,-CELL_QUAR),vec3f(CELL_SIZE,CELL_SIZE,CELL_HALF)),

	//Inner Side Pieces
	Box(vec3f(-CELL_OFFA, 0.0f,-CELL_OFFA),vec3f(CELL_X+CELL_Y,CELL_SIZE,CELL_Y*2)),
	Box(vec3f(-CELL_OFFA, 0.0f, CELL_OFFA),vec3f(CELL_X+CELL_Y,CELL_SIZE,CELL_Y*2)),
	Box(vec3f( CELL_OFFA, 0.0f, CELL_OFFA),vec3f(CELL_X+CELL_Y,CELL_SIZE,CELL_Y*2)),
	Box(vec3f( CELL_OFFA, 0.0f,-CELL_OFFA),vec3f(CELL_X+CELL_Y,CELL_SIZE,CELL_Y*2)),

	//Outer Side Pieces
	Box(vec3f(-CELL_OFFB, 0.0f,-CELL_OFFB),vec3f(CELL_X,CELL_SIZE, CELL_Y)),
	Box(vec3f(-CELL_OFFB, 0.0f, CELL_OFFB),vec3f(CELL_X,CELL_SIZE, CELL_Y)),
	Box(vec3f( CELL_OFFB, 0.0f, CELL_OFFB),vec3f(CELL_X,CELL_SIZE, CELL_Y)),
	Box(vec3f( CELL_OFFB, 0.0f,-CELL_OFFB),vec3f(CELL_X,CELL_SIZE, CELL_Y)),

	//Corner Side Pieces
	Box(vec3f(-CELL_QUAR, 0.0f,-CELL_QUAR),vec3f(CELL_HALF,CELL_SIZE,CELL_HALF)),
	Box(vec3f(-CELL_QUAR, 0.0f, CELL_QUAR),vec3f(CELL_HALF,CELL_SIZE,CELL_HALF)),
	Box(vec3f( CELL_QUAR, 0.0f, CELL_QUAR),vec3f(CELL_HALF,CELL_SIZE,CELL_HALF)),
	Box(vec3f( CELL_QUAR, 0.0f,-CELL_QUAR),vec3f(CELL_HALF,CELL_SIZE,CELL_HALF))
};

const quaternion COLLIDER_ROTATION_TABLE[20]
{
	glm::identity<quaternion>(),
	glm::identity<quaternion>(),
	glm::identity<quaternion>(),
	glm::identity<quaternion>(),

	glm::identity<quaternion>(),
	glm::identity<quaternion>(),
	glm::identity<quaternion>(),
	glm::identity<quaternion>(),

	quaternion(vec3f(0,-RAD_45,  0)),
	quaternion(vec3f(0, RAD_45,  0)),
	quaternion(vec3f(0, RAD_45*3,0)),
	quaternion(vec3f(0, RAD_45*5,0)),

	quaternion(vec3f(0,-RAD_45,  0)),
	quaternion(vec3f(0, RAD_45,  0)),
	quaternion(vec3f(0, RAD_45*3,0)),
	quaternion(vec3f(0, RAD_45*5,0)),

	glm::identity<quaternion>(),
	glm::identity<quaternion>(),
	glm::identity<quaternion>(),
	glm::identity<quaternion>()
};

const quaternion COLLIDER_INVERSE_ROTATION_TABLE[20]
{
	glm::identity<quaternion>(),
	glm::identity<quaternion>(),
	glm::identity<quaternion>(),
	glm::identity<quaternion>(),

	glm::identity<quaternion>(),
	glm::identity<quaternion>(),
	glm::identity<quaternion>(),
	glm::identity<quaternion>(),

	quaternion(vec3f(0, RAD_45,  0)),
	quaternion(vec3f(0,-RAD_45,  0)),
	quaternion(vec3f(0,-RAD_45*3,0)),
	quaternion(vec3f(0,-RAD_45*5,0)),

	quaternion(vec3f(0, RAD_45,  0)),
	quaternion(vec3f(0,-RAD_45,  0)),
	quaternion(vec3f(0,-RAD_45*3,0)),
	quaternion(vec3f(0,-RAD_45*5,0)),

	glm::identity<quaternion>(),
	glm::identity<quaternion>(),
	glm::identity<quaternion>(),
	glm::identity<quaternion>()
};

struct AABB
{
	vec3f min;
	vec3f max;
};

//Shape Key:  0 = Nothing, 1 = Full, 2 = Side, 3 = Inner, 4 = Outer, 5 = Corner
inline byte GetColliderTableIndex(byte shape, byte rotation)
{
	//right here we subtract 1 to shape since we need to have shape = 0
	//to represent no shape for rendering, but the collider table would be empty
	//for all those first entries so we just leave them out
	return rotation + (4*(shape-1));
}


float DistanceToBox(vec3f position, const vec3f& boxPosition,
	 				const vec3f& boxExtents, const quaternion& boxRotation)
{
	// position = boxRotation * position;
	// position -= boxPosition;

	mat4f inverse = glm::identity<mat4f>();
	TRS(inverse, boxPosition, boxRotation);
	inverse = glm::inverse(inverse);
	position = inverse*vec4f(position,1.0);

	//Possible optimization to get rid of abs here
	//since im pretty sure position is always in pos space
	vec3f q = glm::abs(position) - boxExtents;
	return (glm::length(glm::max(q,vec3f(0.0,0.0,0.0))) +
	 		glm::min(glm::max(q.x,glm::max(q.y,q.z)),0.0f));
}

float DistanceToCellShape(vec3f position, vec3f cellPosition,
	 					  byte colliderTableIndex)
{

	return DistanceToBox(position,
		 (COLLIDER_BOX_TABLE[colliderTableIndex].offset + cellPosition),
		 COLLIDER_BOX_TABLE[colliderTableIndex].extents,
		 COLLIDER_ROTATION_TABLE[colliderTableIndex]);
}

inline bool InDirection(vec3f A, vec3f B) { return glm::dot(A,B) > 0; }




// **Courtesy of Inigo Quilez.**
// https://www.iquilezles.org/www/articles/boxfunctions/boxfunctions.htm
// Calcs intersection and exit distances, and normal at intersection.
// The ray must be in box/object space. If you have multiple boxes all
// aligned to the same axis, you can precompute 1/ray. If you have
// multiple boxes but they are not alligned to each other, use the
// "Generic" box intersector bellow this one.
const float MAX_SCALE = FLT_MAX-1;
RayIntersection RayBoxIntersection(vec3f ray, vec3f rayOrigin, const Box& box, vec3f boxPosition,
                                    quaternion boxRotation, quaternion boxInverseRotation)
{
	using namespace glm;
	vec3f rayWorld = ray;
	vec3f rayOriginWorld = rayOrigin;

	ray = boxRotation * ray;
    rayOrigin -= (boxPosition + box.offset);
    rayOrigin = boxRotation * rayOrigin;

	RayIntersection result;
	vec3f inverse = vec3f(1,1,1)/ray;
    //clamp infinite values from division by 0;
    inverse = clamp(inverse,-MAX_SCALE,MAX_SCALE);
	vec3f scaledRayO = inverse*rayOrigin;
	vec3f scaledBox = abs(inverse)*(box.extents*0.5f);
	vec3f f1 = -scaledRayO - scaledBox;
	vec3f f2 = -scaledRayO + scaledBox;

	float32 fracMin = max( max( f1.x, f1.y ), f1.z );
	float32 fracMax = min( min( f2.x, f2.y ), f2.z );

	if( fracMin>fracMax || fracMax<0.0)// no intersection
	{
        result.hit = false;
		result.fraction = -1.0f;
        result.normal = {0,0,0};
		return result;
	}

	result.hit = true;
	result.fraction = fracMin < 0.0f ? fracMax : fracMin;
	result.point = rayOriginWorld + rayWorld*result.fraction;
	result.normal =  glm::rotate(boxRotation, (-sign(ray)*step(f1.yzx(),f1.xyz())*step(f1.zxy(),f1.xyz())));

	return result;
}



// Credit to Inigo Quilez
// from https://iquilezles.org/articles/intersectors/
// axis aligned box centered at the origin, with dimensions "box.extents" and extruded by radious "sphereRadius"
vec3f SphereBoxNormal(vec3f point, vec3f boxHalfSize)
{
	using namespace glm;
	//sign(pos)*normalize(max(abs(pos)-siz,0.0));
    return sign(point)*normalize(max(abs(point)-boxHalfSize, vec3f(0,0,0)));
}

// Credit to Inigo Quilez
// also from https://iquilezles.org/articles/intersectors/
// axis aligned box centered at the origin, with dimensions "box.extents" and extruded by radious "sphereRadius"

//We use this function which is essentially a ray vs rounded box function to test a spherecast against box
//following the Minkowski sum concept
RayIntersection SphereBoxIntersection(vec3f ray, vec3f rayOrigin, float32 sphereRadius, const Box& box,
                                        vec3f boxPosition, quaternion boxRotation, quaternion boxInverseRotation)
{
    // bounding box
    using namespace glm;

	ray = boxRotation * ray;
    rayOrigin -= (boxPosition + box.offset);
    rayOrigin = boxRotation * rayOrigin;

    const vec3f& size = box.extents*0.5f;
	const vec3f rad = vec3f(sphereRadius,sphereRadius,sphereRadius);
    RayIntersection result;
    vec3f m = vec3f(1,1,1)/ray;
    //clamp infinite values from division by 0;
    m = clamp(m,-MAX_SCALE,MAX_SCALE);

    vec3f n = m*rayOrigin;
    vec3f k = abs(m)*(size+rad);
    vec3f t1 = -n - k;
    vec3f t2 = -n + k;
    float32 tN = max( max( t1.x, t1.y ), t1.z );
    float32 tF = min( min( t2.x, t2.y ), t2.z );
    if( tN>tF || tF<0.0f) //no intersection
    {
        result.hit = false;
        result.fraction = -1;
        return result;
    }
    float32 t = tN;

    // convert to first octant
    vec3f pos = rayOrigin+ray*t;
	vec3f point = pos;
    vec3f s = sign(pos);
    rayOrigin  *= s;
    ray  *= s;
    pos *= s;

    // faces
    pos -= size;
    pos = max( pos.xyz(), pos.yzx() );
    if( min(min(pos.x,pos.y),pos.z) < 0.0 )
    {
		if( t < -0.001f )
		{
			result.hit = false;
			return result;
		}
        result.hit = true;
		//t = clamp(t,0.0f,1.0f);
        result.fraction = t;
		vec3 norm = SphereBoxNormal(point, size);
		//TODO: check this math for the point
		point = (point-norm*sphereRadius);
		point = boxInverseRotation*point;
		point += boxPosition+box.offset;
		result.point = point;
		result.normal = glm::rotate(boxInverseRotation, norm);
        return result;
    }

    // some precomputation
    vec3f oc = rayOrigin - size;
    vec3f dd = ray*ray;
    vec3f oo = oc*oc;
    vec3f od = oc*ray;
    float32 ra2 = sphereRadius*sphereRadius;

    t = (float32)1e20;

    // corner
    {
    float32 b = od.x + od.y + od.z;
    float32 c = oo.x + oo.y + oo.z - ra2;
    float32 h = b*b - c;
    if( h>0.0 ) t = -b-sqrt(h);
    }
    // edge X
    {
    float32 a = dd.y + dd.z;
    float32 b = od.y + od.z;
    float32 c = oo.y + oo.z - ra2;
    float32 h = b*b - a*c;
    if( h>0.0 )
    {
        h = (-b-sqrt(h))/a;
        if( h>0.0 && h<t && abs(rayOrigin.x+ray.x*h)<size.x ) t = h;
    }
    }
    // edge Y
    {
    float32 a = dd.z + dd.x;
    float32 b = od.z + od.x;
    float32 c = oo.z + oo.x - ra2;
    float32 h = b*b - a*c;
    if( h>0.0 )
    {
        h = (-b-sqrt(h))/a;
        if( h>0.0 && h<t && abs(rayOrigin.y+ray.y*h)<size.y ) t = h;
    }
    }
    // edge Z
    {
    float32 a = dd.x + dd.y;
    float32 b = od.x + od.y;
    float32 c = oo.x + oo.y - ra2;
    float32 h = b*b - a*c;
    if( h>0.0 )
    {
        h = (-b-sqrt(h))/a;
        if( h>0.0 && h<t && abs(rayOrigin.z+ray.z*h)<size.z ) t = h;
    }
    }

    if( t>(float32)1e19)// || t < -0.5f)
    {
        result.hit = false;
        result.fraction = -1.0f;
        return result;
    }

	result.hit = true;
	//t = clamp(t,0.0f,1.0f);
	result.fraction = t;
	vec3 norm = SphereBoxNormal(point, size);
	point = (point-norm*sphereRadius);
	point = boxInverseRotation*point;
	point += boxPosition+box.offset;
	result.point = point;
	result.normal = glm::rotate(boxInverseRotation, norm);
	return result;
}

//Custom sphere collision against our level.
//Since the level nodes contain information about where the walls are,
//  we can simply get the range of boxes to check against based on the velocity
//  and just test against each box by looking up into a const table of boxes with our
//  level's node data. No instances of boxes or physics objects are necessary  
RayIntersection SphereCollisionCheck(vec3f position, vec3f velocity,const Level& level,
		 float radius, DebugMemory& Debug, bool showCollisionBounds)
{	
	vec3f travelDirection = glm::normalize(velocity);
	float magnitude = glm::length(velocity);
	//position -= travelDirection*(radius*0.99f);
	vec3i from = LevelCellThreeDIndexFromPosition(position)-vec3i(1,0,1);
	vec3i to = LevelCellThreeDIndexFromPosition(position+velocity+(travelDirection*radius))+vec3i(1,0,1);

	from.x = glm::clamp(from.x,0,(int32)level.width-1);
	from.y = glm::clamp(from.y,0,(int32)level.height-1);
	from.z = glm::clamp(from.z,0,(int32)level.width-1);
	to.x = glm::clamp(to.x,0,(int32)level.width-1);
	to.y = glm::clamp(to.y,0,(int32)level.height-1);
	to.z = glm::clamp(to.z,0,(int32)level.width-1);

	uint32 startX = (uint32)glm::min(from.x,to.x);
	uint32 endX   = (uint32)glm::max(from.x,to.x);

	uint32 startY = (uint32)glm::min(from.y, to.y);
	uint32 endY   = (uint32)glm::max(from.y, to.y);

	uint32 startZ = (uint32)glm::min(from.z,to.z);
	uint32 endZ   = (uint32)glm::max(from.z,to.z);

#ifdef DEBUG
    if (showCollisionBounds)
    {

		Debug.LineBuffer.AddBounds(position-vec3f(CELL_HALF,CELL_HALF,CELL_HALF),
    					position+vec3f(CELL_HALF,CELL_HALF,CELL_HALF), vec3f(1,0,0));
		Debug.LineBuffer.AddBounds(position+velocity-vec3f(CELL_HALF,CELL_HALF,CELL_HALF),
    					position+velocity+vec3f(CELL_HALF,CELL_HALF,CELL_HALF), vec3f(1,0,0));
    	Debug.LineBuffer.AddBounds(vec3f(startX,startY,startZ)*CELL_SIZE-vec3f(CELL_HALF,CELL_HALF,CELL_HALF),
    					vec3f(endX,endY,endZ)*CELL_SIZE+vec3f(CELL_HALF,CELL_HALF,CELL_HALF), vec3f(0,1,1));
		Debug.LineBuffer.AddBounds(vec3f(0,0,0)*CELL_SIZE-vec3f(CELL_HALF,CELL_HALF,CELL_HALF),
    					vec3f(level.width-1,0,level.width-1)*CELL_SIZE+vec3f(CELL_HALF,CELL_HALF,CELL_HALF), vec3f(0.5,0.5,0.5));
    }
#endif

	RayIntersection result = { false, 1.0, {0,1,0} };
	RayIntersection test;
	for (uint32 y = startY; y <= endY; y++)
	for (uint32 x = startX; x <= endX; x++)
	for (uint32 z = startZ; z <= endZ; z++)
	{
		NodeCell& cell = level.cells[LinearIndex(x,y,z,level.width)];
		for(byte q = 0; q < 4; q++)
		{
			if (cell.types[q] == 0 || cell.shapes[q] == 0) continue;

			uint32 colIndex = GetColliderTableIndex(cell.shapes[q],q);

			// test = RayBoxIntersection(velocity, position,
			// 			COLLIDER_BOX_TABLE[colIndex], cell.position,
			// 			COLLIDER_ROTATION_TABLE[colIndex],
            //             COLLIDER_INVERSE_ROTATION_TABLE[colIndex]);
             test = SphereBoxIntersection(velocity, position, radius,
			 			COLLIDER_BOX_TABLE[colIndex], cell.position,
			 			COLLIDER_ROTATION_TABLE[colIndex],
                         COLLIDER_INVERSE_ROTATION_TABLE[colIndex]);
			//test.fraction *= magnitude;
			if (test.hit && test.fraction < result.fraction)
                //&& glm::dot(test.normal,travelDirection) < 0 )
			{
				result = test;
#ifdef DEBUG
                if (showCollisionBounds)
                {
    				Debug.LineBuffer.AddBox(COLLIDER_BOX_TABLE[colIndex],
    								cell.position+vec3f(0.0,0.1,0.0),
    								COLLIDER_ROTATION_TABLE[colIndex],{1,0,0});
                }
#endif
			}
		}
	}


#ifdef DEBUG
    if (result.hit && showCollisionBounds)
    {
		Debug.LineBuffer.AddLine(result.point, result.point+result.normal*3.0f, vec3f(0,1,0));
    }
#endif
	return result;
}

#include <chrono>

#include "types.h"
#include "glew/GL/glew.h"
#include "sdl/sdl.h"
const int MAX_DEBUG_LINES = 1000000;
const int MAX_DEBUG_TEXT_CHAR = 1024;

void APIENTRY glDebugOutput(GLenum source,
                            GLenum type,
                            unsigned int id,
                            GLenum severity,
                            GLsizei length,
                            const char *message,
                            const void *userParam)
{
    // ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
    std::string out = "GL Error!:";

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             out += "Source: API, "; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   out += "Source: Window System, "; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: out += "Source: Shader Compiler, "; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     out += "Source: Third Party, "; break;
        case GL_DEBUG_SOURCE_APPLICATION:     out += "Source: Application, "; break;
        case GL_DEBUG_SOURCE_OTHER:           out += "Source: Other, "; break;
    }

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               out += "Type: Error, "; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: out += "Type: Deprecated Behaviour, "; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  out += "Type: Undefined Behaviour, "; break;
        case GL_DEBUG_TYPE_PORTABILITY:         out += "Type: Portability, "; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         out += "Type: Performance, "; break;
        case GL_DEBUG_TYPE_MARKER:              out += "Type: Marker, "; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          out += "Type: Push Group, "; break;
        case GL_DEBUG_TYPE_POP_GROUP:           out += "Type: Pop Group, "; break;
        case GL_DEBUG_TYPE_OTHER:               out += "Type: Other, "; break;
    }

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         out += "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       out += "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          out += "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: out += "Severity: notification"; break;
    }

    LOG_ERROR("%s\n", out.c_str());
}

struct LineVertex
{
    vec3f point;
    vec3f color;
};

struct LineBuffer
{
    uint32 pointCount;
    uint32 capacity;
    LineVertex* vertices;
    uint32 vboID;

    bool AddLine(vec3f pointA, vec3f pointB, vec3f color)
    {
        if (pointCount+2 >= capacity) return false;

        vertices[pointCount].point = pointA;
        vertices[pointCount].color = color;
        vertices[pointCount+1].point = pointB;
        vertices[pointCount+1].color = color;

        pointCount += 2;

        return true;
    }

    void AddBounds(vec3f min, vec3f max, vec3f color)
    {
        AddLine(vec3f(min.x,min.y,min.z),vec3f(min.x,min.y,max.z), color);
        AddLine(vec3f(min.x,min.y,max.z),vec3f(max.x,min.y,max.z), color);
        AddLine(vec3f(max.x,min.y,max.z),vec3f(max.x,min.y,min.z), color);
        AddLine(vec3f(max.x,min.y,min.z),vec3f(min.x,min.y,min.z), color);

        AddLine(vec3f(min.x,max.y,min.z),vec3f(min.x,max.y,max.z), color);
        AddLine(vec3f(min.x,max.y,max.z),vec3f(max.x,max.y,max.z), color);
        AddLine(vec3f(max.x,max.y,max.z),vec3f(max.x,max.y,min.z), color);
        AddLine(vec3f(max.x,max.y,min.z),vec3f(min.x,max.y,min.z), color);

        AddLine(vec3f(min.x,min.y,min.z),vec3f(min.x,max.y,min.z), color);
        AddLine(vec3f(min.x,min.y,max.z),vec3f(min.x,max.y,max.z), color);
        AddLine(vec3f(max.x,min.y,max.z),vec3f(max.x,max.y,max.z), color);
        AddLine(vec3f(max.x,min.y,min.z),vec3f(max.x,max.y,min.z), color);
    }

    void AddBox(Box box, vec3f position, quaternion rotation, vec3f color)
    {
        vec3f half = box.extents/2.0f;
        vec3f p0 = (rotation * vec3f(-half.x,-half.y,-half.z))+position+box.offset;
        vec3f p1 = (rotation * vec3f(-half.x,-half.y, half.z))+position+box.offset;
        vec3f p2 = (rotation * vec3f( half.x,-half.y, half.z))+position+box.offset;
        vec3f p3 = (rotation * vec3f( half.x,-half.y,-half.z))+position+box.offset;
        vec3f p4 = (rotation * vec3f(-half.x, half.y,-half.z))+position+box.offset;
        vec3f p5 = (rotation * vec3f(-half.x, half.y, half.z))+position+box.offset;
        vec3f p6 = (rotation * vec3f( half.x, half.y, half.z))+position+box.offset;
        vec3f p7 = (rotation * vec3f( half.x, half.y,-half.z))+position+box.offset;

        AddLine(p0,p1,color);
        AddLine(p1,p2,color);
        AddLine(p2,p3,color);
        AddLine(p3,p0,color);

        AddLine(p4,p5,color);
        AddLine(p5,p6,color);
        AddLine(p6,p7,color);
        AddLine(p7,p4,color);

        AddLine(p0,p4,color);
        AddLine(p1,p5,color);
        AddLine(p2,p6,color);
        AddLine(p3,p7,color);
    }

    void Clear()
    {
        pointCount = 0;
    }
};

// void DrawTransformHierarchyRecursive(TransformHierarchy* hierarchy,
//                                 LineBuffer& lineBuffer, const vec3f& color,
//                                  vec3f parentPos, quaternion parentRot)
// {
//     quaternion rot = hierarchy->transform->rotation * parentRot;
//     vec3f pos = (rot * hierarchy->transform->position)+parentPos;
//     lineBuffer.AddLine(pos,pos+vec3f(0,1,0), color);
//
//     if (hierarchy->childCount == 0) return;
//
//     for (uint32 i = 0; i < hierarchy->childCount; i++)
//     {
//         DrawTransformHierarchyRecursive(hierarchy->children[i], lineBuffer, color, pos, rot);
//     }
// }

LineBuffer CreateLineBuffer(uint32 capacity)
{
    LineBuffer buffer;
    buffer.vertices = (LineVertex*)SDL_malloc(sizeof(LineVertex)*capacity);
    buffer.capacity = capacity;
    buffer.pointCount = 0;
    glGenBuffers(1, &buffer.vboID);
    glBindBuffer(GL_ARRAY_BUFFER, buffer.vboID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(LineVertex)*capacity, NULL, GL_DYNAMIC_DRAW);

    return buffer;
}

void UploadLineBuffer(const LineBuffer& lineBuffer)
{
	glBindBuffer(GL_ARRAY_BUFFER, lineBuffer.vboID);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(LineVertex)*lineBuffer.pointCount, (void*)lineBuffer.vertices);
}

void DrawLines(const LineBuffer& buffer)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer.vboID);
    glBindVertexBuffer(0, buffer.vboID, 0, sizeof(LineVertex));
    glDrawArrays(GL_LINES, 0, buffer.pointCount);
}

struct Timer
{
    std::chrono::high_resolution_clock::time_point startTime;
    void Mark()
    {
        startTime = std::chrono::high_resolution_clock::now();
    }
    std::chrono::duration<double> ElapsedTime()
    {
        using namespace std::chrono;
        high_resolution_clock::time_point currentTime = high_resolution_clock::now();
    	duration<double> time_span = duration_cast<duration<double>>(
            currentTime - startTime);

        return time_span;
    }
};

struct DebugMemory
{
    LineBuffer LineBuffer;
    QuadBuffer TextBuffer;
    UnicodeGlyphAtlas FontAtlas;
    uint32 TextShader;
    uint32 LineShader;
    uint32 LineVAO;

    bool DebugColliders;
    bool DebugPlayerCollision;
    bool DebugShadowmap;

    void Initialize()
    {
        glLineWidth(3);

    	glGenVertexArrays(1, &LineVAO);
    	glBindVertexArray(LineVAO);

    	ApplyVertexAttribute(0, offsetof(LineVertex, point), 3);
    	ApplyVertexAttribute(1, offsetof(LineVertex, color), 3);

    	LineShader = LoadShader(
    		"resources/shaders/debug/Line_Vertex.glsl",
    		"resources/shaders/debug/Line_Fragment.glsl");

    	LineBuffer = CreateLineBuffer(MAX_DEBUG_LINES);
    	TextBuffer = CreateQuadBufferInHeap(MAX_DEBUG_TEXT_CHAR);
    	TextShader = LoadShader(
    		"resources/shaders/Screenspace_Vertex.glsl",
    		"resources/shaders/BasicText.glsl");
    }
};

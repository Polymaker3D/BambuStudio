#ifndef slic3r_GUI_Selection_hpp_
#define slic3r_GUI_Selection_hpp_

#include "libslic3r/Geometry.hpp"
#include "GLModel.hpp"

#include <set>
#include <optional>

namespace Slic3r {

class Shader;
class Model;
class ModelObject;
class ModelVolume;
class ModelInstance;
class ObjectID;
class GLVolume;
class GLArrow;
class GLCurvedArrow;
class DynamicPrintConfig;
class GLShaderProgram;
#if ENABLE_ENHANCED_PRINT_VOLUME_FIT
class BuildVolume;
#endif // ENABLE_ENHANCED_PRINT_VOLUME_FIT

using GLVolumePtrs = std::vector<GLVolume*>;
using ModelObjectPtrs = std::vector<ModelObject*>;


namespace GUI {
enum ECoordinatesType : unsigned char {
    World = 0,
    Instance,
    Local
};

class TransformationType
{
public:
    enum Enum {
        // Transforming in a world coordinate system
        World = 0,
        // Transforming in a instance coordinate system
        Instance = 1,
        // Transforming in a local coordinate system
        Local = 2,
        // Absolute transformations, allowed in local coordinate system only.
        Absolute = 0,
        // Relative transformations, allowed in both local and world coordinate system.
        Relative = 4,
        // For group selection, the transformation is performed as if the group made a single solid body.
        Joint = 0,
        // For group selection, the transformation is performed on each object independently.
        Independent = 8,

        World_Relative_Joint          = World | Relative | Joint,
        World_Relative_Independent    = World | Relative | Independent,
        Instance_Absolute_Joint       = Instance | Absolute | Joint,
        Instance_Absolute_Independent = Instance | Absolute | Independent,
        Instance_Relative_Joint       = Instance | Relative | Joint,
        Instance_Relative_Independent = Instance | Relative | Independent,
        Local_Absolute_Joint          = Local | Absolute | Joint,
        Local_Absolute_Independent    = Local | Absolute | Independent,
        Local_Relative_Joint          = Local | Relative | Joint,
        Local_Relative_Independent    = Local | Relative | Independent,
    };

    TransformationType() : m_value(World) {}
    TransformationType(Enum value) : m_value(value) {}
    TransformationType &operator=(Enum value)
    {
        m_value = value;
        return *this;
    }

    Enum operator()() const { return m_value; }
    bool has(Enum v) const { return ((unsigned int) m_value & (unsigned int) v) != 0; }

    void set_world()
    {
        this->remove(Instance);
        this->remove(Local);
    }
    void set_instance()
    {
        this->remove(Local);
        this->add(Instance);
    }
    void set_local()
    {
        this->remove(Instance);
        this->add(Local);
    }
    void set_absolute() { this->remove(Relative); }
    void set_relative() { this->add(Relative); }
    void set_joint() { this->remove(Independent); }
    void set_independent() { this->add(Independent); }

    bool world() const { return !this->has(Instance) && !this->has(Local); }
    bool instance() const { return this->has(Instance); }
    bool local() const { return this->has(Local); }
    bool absolute() const { return !this->has(Relative); }
    bool relative() const { return this->has(Relative); }
    bool joint() const { return !this->has(Independent); }
    bool independent() const { return this->has(Independent); }

private:
    void add(Enum v) { m_value = Enum((unsigned int)m_value | (unsigned int)v); }
    void remove(Enum v) { m_value = Enum((unsigned int)m_value & (~(unsigned int)v)); }

    Enum    m_value;
};

class Selection
{
public:
    typedef std::set<unsigned int> IndicesList;

    enum EMode : unsigned char
    {
        Volume,
        Instance
    };

    enum EType : unsigned char
    {
        Invalid,
        Empty,
        WipeTower,
        SingleModifier,
        MultipleModifier,
        SingleVolume,
        MultipleVolume,
        SingleFullObject,
        MultipleFullObject,
        SingleFullInstance,
        MultipleFullInstance,
        Mixed
    };

private:
    struct VolumeCache
    {
    private:
        struct TransformCache
        {
            Vec3d position;
            Vec3d rotation;
            Vec3d scaling_factor;
            Vec3d mirror;
            Transform3d rotation_matrix;
            Transform3d scale_matrix;
            Transform3d mirror_matrix;
            Geometry::Transformation full_tran;

            TransformCache();
            explicit TransformCache(const Geometry::Transformation& transform);
        };

        TransformCache m_volume;
        TransformCache m_instance;

    public:
        VolumeCache() = default;
        VolumeCache(const Geometry::Transformation& volume_transform, const Geometry::Transformation& instance_transform);

        const Vec3d& get_volume_position() const { return m_volume.position; }
        const Vec3d& get_volume_rotation() const { return m_volume.rotation; }
        const Vec3d& get_volume_scaling_factor() const { return m_volume.scaling_factor; }
        const Vec3d& get_volume_mirror() const { return m_volume.mirror; }
        const Transform3d& get_volume_rotation_matrix() const { return m_volume.rotation_matrix; }
        const Transform3d& get_volume_scale_matrix() const { return m_volume.scale_matrix; }
        const Transform3d& get_volume_mirror_matrix() const { return m_volume.mirror_matrix; }
        const Transform3d &get_volume_full_matrix() const { return m_volume.full_tran.get_matrix(); }

        const Vec3d& get_instance_position() const { return m_instance.position; }
        const Vec3d& get_instance_rotation() const { return m_instance.rotation; }
        const Vec3d& get_instance_scaling_factor() const { return m_instance.scaling_factor; }
        const Vec3d& get_instance_mirror() const { return m_instance.mirror; }
        const Transform3d& get_instance_rotation_matrix() const { return m_instance.rotation_matrix; }
        const Transform3d& get_instance_scale_matrix() const { return m_instance.scale_matrix; }
        const Transform3d& get_instance_mirror_matrix() const { return m_instance.mirror_matrix; }
        const Transform3d &get_instance_full_matrix() const { return m_instance.full_tran.get_matrix(); }

        const Geometry::Transformation &get_volume_transform() const { return m_volume.full_tran; }
        const Geometry::Transformation &get_instance_transform() const { return m_instance.full_tran; }
    };

public:
    typedef std::map<unsigned int, VolumeCache> VolumesCache;
    typedef std::set<int> InstanceIdxsList;
    typedef std::map<int, InstanceIdxsList> ObjectIdxsToInstanceIdxsMap;

    class Clipboard
    {
        // Model is stored through a pointer to avoid including heavy Model.hpp.
        // It is created in constructor.
        std::unique_ptr<Model> m_model;

        Selection::EMode m_mode;

    public:
        Clipboard();

        void reset();
        bool is_empty() const;

        bool is_sla_compliant() const;

        ModelObject* add_object();
        ModelObject* get_object(unsigned int id);
        const ModelObjectPtrs& get_objects() const;

        Selection::EMode get_mode() const { return m_mode; }
        void set_mode(Selection::EMode mode) { m_mode = mode; }
    };

private:
    struct Cache
    {
        // Cache of GLVolume derived transformation matrices, valid during mouse dragging.
        VolumesCache volumes_data;
        // Center of the dragged selection, valid during mouse dragging.
        Vec3d dragging_center;
        // Map from indices of ModelObject instances in Model::objects
        // to a set of indices of ModelVolume instances in ModelObject::instances
        // Here the index means a position inside the respective std::vector, not ObjectID.
        ObjectIdxsToInstanceIdxsMap content;
        // List of ids of the volumes which are sinking when starting dragging
        std::vector<unsigned int> sinking_volumes;
        Vec3d                     rotation_pivot;
    };

    // Volumes owned by GLCanvas3D.
    GLVolumePtrs* m_volumes;
    // Model, not owned.
    Model* m_model;

    bool m_enabled;
    bool m_valid;
    EMode m_mode;
    EType m_type;
    // set of indices to m_volumes
    IndicesList m_list;
    Cache m_cache;
    Clipboard m_clipboard;
    std::optional<BoundingBoxf3> m_bounding_box;
    // Bounding box of a selection, with no instance scaling applied. This bounding box
    // is useful for absolute scaling of tilted objects in world coordinate space.
    std::optional<BoundingBoxf3> m_unscaled_instance_bounding_box;
    std::optional<BoundingBoxf3> m_scaled_instance_bounding_box;
    // Bounding box of a single full instance selection, in world coordinates, with no instance scaling applied.
    // Modifiers are taken in account
    std::optional<BoundingBoxf3> m_full_unscaled_instance_bounding_box;
    // Bounding box of a single full instance selection, in world coordinates.
    // Modifiers are taken in account
    std::optional<BoundingBoxf3> m_full_scaled_instance_bounding_box;
    // Bounding box of a single full instance selection, in local coordinates, with no instance scaling applied.
    // Modifiers are taken in account
    std::optional<BoundingBoxf3> m_full_unscaled_instance_local_bounding_box;
    // Bounding box aligned to the axis of the currently selected reference system (World/Object/Part)
    // and transform to place and orient it in world coordinates
    std::optional<std::pair<BoundingBoxf3, Transform3d>> m_bounding_box_in_current_reference_system;

    std::optional<std::pair<Vec3d, double>> m_bounding_sphere;
#if ENABLE_RENDER_SELECTION_CENTER
    mutable GLModel m_vbo_sphere;
#endif // ENABLE_RENDER_SELECTION_CENTER

    GLModel m_arrow;
    GLModel m_curved_arrow;

    float m_scale_factor;
    bool m_dragging;

    // BBS
    EMode m_volume_selection_mode{ Instance };
    bool m_volume_selection_locked { false };
    std::vector<Transform3d> m_trafo_matrices;

    mutable GLModel m_bounding_box_model;
    mutable GLModel m_sidebar_layers_hints_model;
public:
    Selection();

    void set_volumes(GLVolumePtrs* volumes);
    bool init();

    bool is_enabled() const { return m_enabled; }
    void set_enabled(bool enable) { m_enabled = enable; }

    Model* get_model() const { return m_model; }
    void set_model(Model* model);

    EMode get_mode() const { return m_mode; }
    void  set_mode(EMode mode);

    int query_real_volume_idx_from_other_view(unsigned int object_idx, unsigned int instance_idx, unsigned int model_volume_idx);
    void add(unsigned int volume_idx, bool as_single_selection = true, bool check_for_already_contained = false);
    void remove(unsigned int volume_idx);

    void add_object(unsigned int object_idx, bool as_single_selection = true);
    void remove_object(unsigned int object_idx);

    void add_instance(unsigned int object_idx, unsigned int instance_idx, bool as_single_selection = true);
    void remove_instance(unsigned int object_idx, unsigned int instance_idx);

    void add_volume(unsigned int object_idx, unsigned int volume_idx, int instance_idx, bool as_single_selection = true);
    void remove_volume(unsigned int object_idx, unsigned int volume_idx);

    void add_volumes(EMode mode, const std::vector<unsigned int>& volume_idxs, bool as_single_selection = true);
    void remove_volumes(EMode mode, const std::vector<unsigned int>& volume_idxs);

    //BBS
    ModelVolume *                   get_selected_single_volume(int &out_object_idx, int &out_volume_idx);
    ModelObject *                   get_selected_single_object(int &out_object_idx);
    const ModelInstance *           get_selected_single_intance();
    const std::vector<Transform3d> &get_all_tran_of_selected_volumes();
    void add_curr_plate();
    void add_object_from_idx(std::vector<int>& object_idxs);
    void remove_curr_plate();
    void clone(int numbers = 1);
    void center();
    void center_plate(const int plate_idx);
    void set_printable(bool printable);

    void add_all();
    void remove_all();

    // To be called after Undo or Redo once the volumes are updated.
    void set_deserialized(EMode mode, const std::vector<std::pair<size_t, size_t>> &volumes_and_instances);

    // Update the selection based on the new instance IDs.
	void instances_changed(const std::vector<size_t> &instance_ids_selected);
    // Update the selection based on the map from old indices to new indices after m_volumes changed.
    // If the current selection is by instance, this call may select newly added volumes, if they belong to already selected instances.
    void volumes_changed(const std::vector<size_t> &map_volume_old_to_new);
    void clear();

    bool is_empty() const { return m_type == Empty; }
    bool is_wipe_tower() const { return m_type == WipeTower; }
    bool is_any_modifier() const { return is_single_modifier() || is_multiple_modifier(); }
    bool is_single_modifier() const { return m_type == SingleModifier; }
    bool is_multiple_modifier() const { return m_type == MultipleModifier; }
    bool is_single_full_instance() const;
    bool is_multiple_full_instance() const { return m_type == MultipleFullInstance; }
    bool is_single_full_object() const { return m_type == SingleFullObject; }
    bool is_multiple_full_object() const { return m_type == MultipleFullObject; }
    bool is_single_volume() const { return m_type == SingleVolume; }
    bool is_multiple_volume() const { return m_type == MultipleVolume; }
    bool is_any_volume() const { return is_single_volume() || is_multiple_volume(); }
    bool is_single_volume_or_modifier() const { return is_single_volume() || is_single_modifier(); }
    bool is_any_connector() const;
    bool is_any_cut_volume() const;
    bool is_mixed() const { return m_type == Mixed; }
    bool is_from_single_instance() const { return get_instance_idx() != -1; }
    bool is_from_single_object() const;
    bool is_sla_compliant() const;
    bool is_instance_mode() const { return m_mode == Instance; }
    bool has_emboss_shape() const;

    bool contains_volume(unsigned int volume_idx) const { return m_list.find(volume_idx) != m_list.end(); }
    // returns true if the selection contains all the given indices
    bool contains_all_volumes(const std::vector<unsigned int>& volume_idxs) const;
    // returns true if the selection contains at least one of the given indices
    bool contains_any_volume(const std::vector<unsigned int>& volume_idxs) const;
    // returns true if the selection contains all and only the given indices
    bool matches(const std::vector<unsigned int>& volume_idxs) const;

    bool requires_uniform_scale() const;

    // Returns the the object id if the selection is from a single object, otherwise is -1
    int get_object_idx() const;
    // Returns the instance id if the selection is from a single object and from a single instance, otherwise is -1
    int get_instance_idx() const;
    // Returns the indices of selected instances.
    // Can only be called if selection is from a single object.
    const InstanceIdxsList& get_instance_idxs() const;

    const IndicesList& get_volume_idxs() const { return m_list; }
    const GLVolume* get_volume(unsigned int volume_idx) const;
    GLVolume*      get_volume(unsigned int volume_idx);
    const GLVolume* get_volume_by_object_volumn_id(unsigned int volume_id) const;
    const GLVolume* get_first_volume() const;
    const ObjectIdxsToInstanceIdxsMap& get_content() const { return m_cache.content; }

    unsigned int volumes_count() const { return (unsigned int)m_list.size(); }
    const BoundingBoxf3& get_bounding_box() const;
    // Bounding box of a selection, with no instance scaling applied. This bounding box
    // is useful for absolute scaling of tilted objects in world coordinate space.
    const BoundingBoxf3& get_unscaled_instance_bounding_box() const;
    const BoundingBoxf3& get_scaled_instance_bounding_box() const;
    // Bounding box of a single full instance selection, in world coordinates, with no instance scaling applied.
    // Modifiers are taken in account
    const BoundingBoxf3 &get_full_unscaled_instance_bounding_box() const;
    // Bounding box of a single full instance selection, in world coordinates.
    // Modifiers are taken in account
    const BoundingBoxf3 &get_full_scaled_instance_bounding_box() const;
    // Bounding box of a single full instance selection, in local coordinates, with no instance scaling applied.
    // Modifiers are taken in account
    const BoundingBoxf3 &get_full_unscaled_instance_local_bounding_box() const;
    // Returns the bounding box aligned to the axes of the currently selected reference system (World/Object/Part)
    // and the transform to place and orient it in world coordinates
    const std::pair<BoundingBoxf3, Transform3d> &get_bounding_box_in_current_reference_system() const;
    // Returns the bounding box aligned to the axes of the given reference system
    // and the transform to place and orient it in world coordinates
    std::pair<BoundingBoxf3, Transform3d> get_bounding_box_in_reference_system(ECoordinatesType type) const;

    void start_dragging();
    void stop_dragging() { m_dragging = false; }
    bool is_dragging() const { return m_dragging; }
    // Returns the bounding sphere: first = center, second = radius
    const std::pair<Vec3d, double> get_bounding_sphere() const;

    void setup_cache();
    void translate(const Vec3d &displacement, TransformationType transformation_type);//new
    void move_to_center(const Vec3d& displacement, bool local = false);
    void rotate(const Vec3d& rotation, TransformationType transformation_type);
    void flattening_rotate(const Vec3d& normal);
    void scale(const Vec3d& scale, TransformationType transformation_type);
#if ENABLE_ENHANCED_PRINT_VOLUME_FIT
    void scale_to_fit_print_volume(const BuildVolume& volume);
#else
    void scale_to_fit_print_volume(const DynamicPrintConfig& config);
#endif // ENABLE_ENHANCED_PRINT_VOLUME_FIT
    void scale_and_translate(const Vec3d &scale, const Vec3d &world_translation, TransformationType transformation_type);
    void mirror(Axis axis, TransformationType transformation_type);

    void translate(unsigned int object_idx, const Vec3d& displacement);
    void translate(unsigned int object_idx, unsigned int instance_idx, const Vec3d& displacement);
    void translate(unsigned int object_idx, unsigned int instance_idx, unsigned int volume_idx, const Vec3d &displacement);

    void rotate(unsigned int object_idx, unsigned int instance_idx, const Transform3d &overwrite_tran);
    void rotate(unsigned int object_idx, unsigned int instance_idx, unsigned int volume_idx, const Transform3d &overwrite_tran);
    //BBS: add partplate related logic
    void notify_instance_update(int object_idx, int instance_idx);
    // BBS
    EMode get_volume_selection_mode(){ return m_volume_selection_mode;}
    void set_volume_selection_mode(EMode mode) { if (!m_volume_selection_locked) m_volume_selection_mode = mode; }
    void lock_volume_selection_mode() { m_volume_selection_locked = true; }
    void unlock_volume_selection_mode() { m_volume_selection_locked = false; }

    void erase();

    void render(float scale_factor = 1.0) const;
#if ENABLE_RENDER_SELECTION_CENTER
    void render_center(bool gizmo_is_dragging) const;
#endif // ENABLE_RENDER_SELECTION_CENTER
    //BBS: GUI refactor: add uniform scale from gizmo
    void render_sidebar_hints(const std::string& sidebar_field, bool uniform_scale) const;

    bool requires_local_axes() const;

    void render_bounding_box(const BoundingBoxf3& box, float* color, float scale) {
        m_scale_factor = scale;
        render_bounding_box(box, color);
    }

    //BBS
    void cut_to_clipboard();
    void copy_to_clipboard();
    void paste_from_clipboard();
    //BBS get selected object instance lists
    std::set<std::pair<int, int>> get_selected_object_instances();

    const Clipboard& get_clipboard() const { return m_clipboard; }

    void fill_color(int  extruder_id);

    // returns the list of idxs of the volumes contained into the object with the given idx
    std::vector<unsigned int> get_volume_idxs_from_object(unsigned int object_idx) const;
    // returns the list of idxs of the volumes contained into the instance with the given idxs
    std::vector<unsigned int> get_volume_idxs_from_instance(unsigned int object_idx, unsigned int instance_idx) const;
    // returns the idx of the volume corresponding to the volume with the given idxs
    std::vector<unsigned int> get_volume_idxs_from_volume(unsigned int object_idx, unsigned int instance_idx, unsigned int volume_idx) const;
    // returns the list of idxs of the volumes contained in the selection but not in the given list
    std::vector<unsigned int> get_missing_volume_idxs_from(const std::vector<unsigned int>& volume_idxs) const;
    // returns the list of idxs of the volumes contained in the given list but not in the selection
    std::vector<unsigned int> get_unselected_volume_idxs_from(const std::vector<unsigned int>& volume_idxs) const;

private:
    void update_valid();
    void update_type();
    void set_caches();
    void do_add_volume(unsigned int volume_idx);
    void do_add_volumes(const std::vector<unsigned int>& volume_idxs);
    void do_remove_volume(unsigned int volume_idx);
    void do_remove_instance(unsigned int object_idx, unsigned int instance_idx);
    void do_remove_object(unsigned int object_idx);
    void set_bounding_boxes_dirty() {
        m_bounding_box.reset();
        m_unscaled_instance_bounding_box.reset(); m_scaled_instance_bounding_box.reset();
        m_full_unscaled_instance_bounding_box.reset();
        m_full_scaled_instance_bounding_box.reset();
        m_full_unscaled_instance_local_bounding_box.reset();
        m_bounding_box_in_current_reference_system.reset();
        m_bounding_sphere.reset();
    }
    void render_selected_volumes() const;
    void render_synchronized_volumes() const;
    void render_bounding_box(const BoundingBoxf3& box, float* color) const;
    void render_sidebar_position_hints(const std::string& sidebar_field, GLShaderProgram& shader, const Transform3d& model_matrix) const;
    void render_sidebar_rotation_hints(const std::string& sidebar_field, GLShaderProgram& shader, const Transform3d& model_matrix) const;
    //BBS: GUI refactor: add uniform_scale from gizmo
    void render_sidebar_scale_hints(const std::string& sidebar_field, bool gizmo_uniform_scale, GLShaderProgram& shader, const Transform3d& model_matrix) const;
    void render_sidebar_layers_hints(GLShaderProgram& shader, const std::string& sidebar_field) const;
    void init_bounding_box_model() const;

public:
    enum class SyncRotationType {
        // Do not synchronize rotation. Either not rotating at all, or rotating by world Z axis.
        NONE = 0,
        // Synchronize after rotation by an axis not parallel with Z.
        GENERAL = 1,
        // Synchronize after rotation reset.
        RESET = 2
    };
    void synchronize_unselected_instances(SyncRotationType sync_rotation_type);
    void synchronize_unselected_volumes();

private:
    void ensure_on_bed();
    void ensure_not_below_bed();
    bool is_from_fully_selected_instance(unsigned int volume_idx) const;

    void paste_volumes_from_clipboard();
    void paste_objects_from_clipboard();

    void transform_instance_relative(
        GLVolume &volume, const VolumeCache &volume_data, TransformationType transformation_type, const Transform3d &transform, const Vec3d &world_pivot);
    void transform_volume_relative(
        GLVolume &volume, const VolumeCache &volume_data, TransformationType transformation_type, const Transform3d &transform, const Vec3d &world_pivot);
};
ModelVolume *   get_selected_volume(const Selection &selection);
const GLVolume *get_selected_gl_volume(const Selection &selection);

ModelVolume *get_selected_volume(const ObjectID &volume_id, const Selection &selection);
ModelVolume *get_volume(const ObjectID &volume_id, const Selection &selection);
} // namespace GUI
} // namespace Slic3r

#endif // slic3r_GUI_Selection_hpp_

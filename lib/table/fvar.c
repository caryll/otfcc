#include "fvar.h"

#include "support/util.h"

// fvar instance tuple
// fvar instance
static INLINE void initFvarInstance(fvar_Instance *inst) {
	memset(inst, 0, sizeof(*inst));
	iVV.init(&inst->coordinates);
}
static INLINE void disposeFvarInstance(fvar_Instance *inst) {
	iVV.dispose(&inst->coordinates);
}
caryll_standardType(fvar_Instance, fvar_iInstance, initFvarInstance, disposeFvarInstance);
caryll_standardVectorImpl(fvar_InstanceList, fvar_Instance, fvar_iInstance, fvar_iInstanceList);
// table fvar
static INLINE void initFvar(table_fvar *fvar) {
	memset(fvar, 0, sizeof(*fvar));
	vf_iAxes.init(&fvar->axes);
	fvar_iInstanceList.init(&fvar->instances);
}
static INLINE void disposeFvar(table_fvar *fvar) {
	vf_iAxes.dispose(&fvar->axes);
	fvar_iInstanceList.dispose(&fvar->instances);
}
caryll_standardRefType(table_fvar, table_iFvar, initFvar, disposeFvar);

// Local typedefs for parsing axis record
struct FVARHeader {
	uint16_t majorVersion;
	uint16_t minorVersion;
	uint16_t axesArrayOffset;
	uint16_t reserved1;
	uint16_t axisCount;
	uint16_t axisSize;
	uint16_t instanceCount;
	uint16_t instanceSize;
};

struct VariationAxisRecord {
	uint32_t axisTag;
	f16dot16 minValue;
	f16dot16 defaultValue;
	f16dot16 maxValue;
	uint16_t flags;
	uint16_t axisNameID;
};

struct InstanceRecord {
	uint16_t subfamilyNameID;
	uint16_t flags;
	f16dot16 coordinates[];
};

table_fvar *otfcc_readFvar(const otfcc_Packet packet, const otfcc_Options *options) {
	table_fvar *fvar = NULL;
	FOR_TABLE('fvar', table) {
		font_file_pointer data = table.data;
		if (table.length < sizeof(struct FVARHeader)) goto FAIL;

		struct FVARHeader *header = (struct FVARHeader *)data;
		if (be16(header->majorVersion) != 1) goto FAIL;
		if (be16(header->minorVersion) != 0) goto FAIL;
		if (be16(header->axesArrayOffset) == 0) goto FAIL;
		if (be16(header->axisCount) == 0) goto FAIL;
		if (be16(header->axisSize) != sizeof(struct VariationAxisRecord)) goto FAIL;
		uint16_t nAxes = be16(header->axisCount);
		uint16_t instanceSizeWithoutPSNID = 4 + nAxes * sizeof(f16dot16);
		uint16_t instanceSizeWithPSNID = 2 + instanceSizeWithoutPSNID;
		if (be16(header->instanceSize) != instanceSizeWithoutPSNID &&
		    be16(header->instanceSize) != instanceSizeWithPSNID)
			goto FAIL;
		if (table.length < be16(header->axesArrayOffset) +
		                       sizeof(struct VariationAxisRecord) * nAxes +
		                       be16(header->instanceSize) * be16(header->instanceCount))
			goto FAIL;

		fvar = table_iFvar.create();

		// parse axes
		struct VariationAxisRecord *axisRecord =
		    (struct VariationAxisRecord *)(data + be16(header->axesArrayOffset));
		for (uint16_t j = 0; j < nAxes; j++) {
			vf_Axis axis = {.tag = be32(axisRecord->axisTag),
			                .minValue = otfcc_from_fixed(be32(axisRecord->minValue)),
			                .defaultValue = otfcc_from_fixed(be32(axisRecord->defaultValue)),
			                .maxValue = otfcc_from_fixed(be32(axisRecord->maxValue)),
			                .flags = be16(axisRecord->flags),
			                .axisNameID = be16(axisRecord->axisNameID)};
			vf_iAxes.push(&fvar->axes, axis);
			axisRecord++;
		}

		// parse instances
		uint16_t nInstances = be16(header->instanceCount);
		bool hasPostscriptNameID = be16(header->instanceSize) == instanceSizeWithPSNID;
		struct InstanceRecord *instance = (struct InstanceRecord *)axisRecord;
		for (uint16_t j = 0; j < nInstances; j++) {
			fvar_Instance inst;
			fvar_iInstance.init(&inst);
			inst.subfamilyNameID = be16(instance->subfamilyNameID);
			inst.flags = be16(instance->flags);
			for (uint16_t k = 0; k < nAxes; k++) {
				iVV.push(&inst.coordinates, otfcc_from_fixed(be32(instance->coordinates[k])));
			}
			iVV.shrinkToFit(&inst.coordinates);
			if (hasPostscriptNameID) {
				inst.postScriptNameID =
				    be16(*(uint16_t *)(((font_file_pointer)instance) + instanceSizeWithoutPSNID));
			}
			fvar_iInstanceList.push(&fvar->instances, inst);
			instance = (struct InstanceRecord *)(((font_file_pointer)instance) +
			                                     be16(header->instanceSize));
		}
		vf_iAxes.shrinkToFit(&fvar->axes);
		fvar_iInstanceList.shrinkToFit(&fvar->instances);

		return fvar;

	FAIL:
		logWarning("table 'fvar' corrupted.\n");
		table_iFvar.free(fvar);
		fvar = NULL;
	}
	return NULL;
}

void otfcc_dumpFvar(const table_fvar *table, json_value *root, const otfcc_Options *options) {
	if (!table) return;
	loggedStep("fvar") {
		json_value *t = json_object_new(2);
		// dump axes
		json_value *_axes = json_object_new(table->axes.length);
		foreach (vf_Axis *axis, table->axes) {
			char *tag = tag2str(axis->tag);
			json_value *_axis = json_object_new(5);
			json_object_push(_axis, "minValue", json_double_new(axis->minValue));
			json_object_push(_axis, "defaultValue", json_double_new(axis->defaultValue));
			json_object_push(_axis, "maxValue", json_double_new(axis->maxValue));
			json_object_push(_axis, "flags", json_integer_new(axis->flags));
			json_object_push(_axis, "axisNameID", json_integer_new(axis->axisNameID));
			json_object_push(_axes, tag, _axis);
			tag_free(tag);
		}
		json_object_push(t, "axes", _axes);
		// dump instances
		json_value *_instances = json_array_new(table->instances.length);
		foreach (fvar_Instance *instance, table->instances) {
			json_value *_instance = json_object_new(4);
			json_object_push(_instance, "subfamilyNameID",
			                 json_integer_new(instance->subfamilyNameID));
			if (instance->postScriptNameID) {
				json_object_push(_instance, "postScriptNameID",
				                 json_integer_new(instance->postScriptNameID));
			}
			json_object_push(_instance, "flags", json_integer_new(instance->flags));
			json_object_push(_instance, "coordinates", json_new_VVp(&instance->coordinates));
			json_array_push(_instances, _instance);
		}
		json_object_push(t, "instances", _instances);
		json_object_push(root, "fvar", t);
	}
}

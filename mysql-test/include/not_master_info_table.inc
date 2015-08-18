if (`SELECT @@GLOBAL.primary_info_repository = 'TABLE'`)
{
     skip Test cannot run with primary.info repository table;
}
